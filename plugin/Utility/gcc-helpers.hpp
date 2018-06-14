#pragma once

#include "general-helpers.hpp"
#include "types.hpp"

namespace c2ocaml {
namespace frontend {
namespace util {

inline std::string gcc_str(gimple *g, bool is_seq = false) {
  // We need some temporary space
  static const int SIZE = 1024 * 1024;
  static char buffer[SIZE];

  // Clear buffer
  memset(buffer, 0, SIZE);

  // Get a in-memory file descriptor to put
  // the string in
  auto mem = fmemopen(buffer, SIZE, "w");

  // Use gimple-pretty-printer's abilities
  if (is_seq) {
    print_gimple_seq(mem, g, 0, TDF_VOPS | TDF_MEMSYMS);
  } else {
    print_gimple_stmt(mem, g, 0, TDF_VOPS | TDF_MEMSYMS);
  }

  // Trim the newline it adds
  buffer[strlen(buffer) - 1] = '\0';

  // Make it a string
  auto as_str = std::string(buffer);

  // Close in mem file
  fclose(mem);

  return as_str;
}

inline std::string gcc_str(tree t) {
  // We need some temporary space
  static const int SIZE = 1024 * 1024;
  static char buffer[SIZE];

  // Clear buffer
  memset(buffer, 0, SIZE);

  // Get a in-memory file descriptor to put
  // the string in
  auto mem = fmemopen(buffer, SIZE, "w");

  // Use tree-pretty-printer's abilities
  print_generic_stmt(mem, t, 0);

  // Trim the newline it adds
  buffer[strlen(buffer) - 1] = '\0';

  // Make it a string
  auto as_str = std::string(buffer);

  // Close in mem file
  fclose(mem);

  return as_str;
}

inline std::string get_source_lines(const std::string &file_path,
                                    int64_t from_line, int64_t to_line) {
  // We need some temporary space
  static const int SIZE = 1024 * 1024 * 100;
  static char buffer[SIZE];

  // Clear buffer
  memset(buffer, 0, SIZE);

  int total_size = 0;

  // Grab lines
  while (from_line <= to_line) {
    int line_size = 0;

    const char *buff =
        location_get_source_line(file_path.c_str(), from_line, &line_size);

    strncat(buffer, buff, line_size);
    strcat(buffer, "\n"); // Don't forget the newline

    from_line += 1;
    total_size += line_size;
  }

  return std::string(buffer, total_size);
}

template <typename func> inline void for_each_bb(types::gcc_func fun, func f) {
  types::gcc_bb bb;

  f(ENTRY_BLOCK_PTR_FOR_FN(fun), ENTRY_BLOCK_PTR_FOR_FN(fun)->index);

  FOR_EACH_BB_FN(bb, fun) { f(bb, bb->index); }

  f(EXIT_BLOCK_PTR_FOR_FN(fun), EXIT_BLOCK_PTR_FOR_FN(fun)->index);
}

template <typename func> inline void for_each_loop(types::gcc_func fn, func f) {
  types::gcc_loop lp;
  int32_t flags = 0;
  FOR_EACH_LOOP_FN(fn, lp, flags) { f(lp); }
}

template <typename func>
inline void for_each_loop_exit(types::gcc_loop lp, func f) {
  auto exit_edges = get_loop_exit_edges(lp);
  auto exit_edge_index = 0;

  types::gcc_edge exit_edge;

  FOR_EACH_VEC_ELT(exit_edges, exit_edge_index, exit_edge) { f(exit_edge); };
}

template <typename func> inline void for_each_stmt(types::gcc_bb bb, func f) {
  auto phis = phi_nodes(bb);

  gimple_stmt_iterator gsi;

  for (gsi = gsi_start(phis); !gsi_end_p(gsi); gsi_next(&gsi)) {
    f(gsi_stmt(gsi));
  }

  for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
    f(gsi_stmt(gsi));
  }
}

template <typename func>
inline void for_each_real_phi(types::gcc_bb bb, func f) {
  auto phis = phi_nodes(bb);

  gimple_stmt_iterator gsi;

  for (gsi = gsi_start(phis); !gsi_end_p(gsi); gsi_next(&gsi)) {
    // Ignore .MEM phi assignments
    if (virtual_operand_p(gimple_phi_result(gsi_stmt(gsi)))) {
      continue;
    }

    // call the worker
    f(gsi_stmt(gsi));
  }
}

template <typename func>
inline void for_each_param(types::gcc_func fun, func f) {
  int32_t idx = 0;
  for (auto decl = DECL_ARGUMENTS(fun->decl); decl; decl = DECL_CHAIN(decl)) {
    f(decl, idx++);
  }
}

template <typename func>
inline void for_each_local(types::gcc_func fun, func f) {
  int32_t idx = 0, i = 0;
  types::gcc_tree var;

  FOR_EACH_LOCAL_DECL(fun, i, var) { f(var, idx++); }
}

template <typename func>
inline void for_each_bb_succ(types::gcc_bb bb, func f) {
  types::gcc_edge e;
  types::gcc_edge_iter ei;

  FOR_EACH_EDGE(e, ei, bb->succs) { f(e); }
}

template <typename func>
inline void for_each_bb_pred(types::gcc_bb bb, func f) {
  types::gcc_edge e;
  types::gcc_edge_iter ei;

  FOR_EACH_EDGE(e, ei, bb->preds) { f(e); }
}

template <typename func> inline void traverse_cfg(types::gcc_func fun, func f) {
  auto currbb = ENTRY_BLOCK_PTR_FOR_FN(fun);
  auto worklist = std::stack<types::gcc_bb>();

  std::vector<int32_t> visited;
  types::gcc_edge e;
  types::gcc_edge_iter ei;

  // Add to visit list
  worklist.push(currbb);

  while (!worklist.empty()) {
    currbb = worklist.top();
    worklist.pop();

    auto srcIdx = currbb->index;

    // Make sure we haven't been here already
    auto found = std::find(visited.begin(), visited.end(), srcIdx);
    if (found != visited.end()) {
      continue;
    }

    visited.push_back(srcIdx);

    FOR_EACH_EDGE(e, ei, currbb->succs) {
      auto dstIdx = e->dest->index;

      f(srcIdx, dstIdx, e->flags);

      worklist.push(e->dest);
    }
  }
}

inline std::string gcc_str_code_inverse(int32_t code) {
  switch (code) {
  case GT_EXPR:
    return "blte";
  case GE_EXPR:
    return "blt";
  case LT_EXPR:
    return "bgte";
  case LE_EXPR:
    return "bgt";
  case NE_EXPR:
    return "beq";
  case EQ_EXPR:
    return "bneq";
  case UNGT_EXPR:
    return "bungt_inv";
  case UNGE_EXPR:
    return "bungte_inv";
  case UNLT_EXPR:
    return "bunlt_inv";
  case UNLE_EXPR:
    return "bunlte_inv";
  case UNEQ_EXPR:
    return "buneq_inv";
  case UNORDERED_EXPR:
    return "unordered_inv";
  case ORDERED_EXPR:
    return "ordered_inv";
  default: {
    std::cerr << "Unrepresentable(\"Bad Op Code To Inverse: " << code << "\""
              << std::endl;
    assert(false);
  }
  }
}

inline std::string gcc_str_code(enum tree_code code) {
  switch (code) {
  // binary arithmetic exprs
  case PLUS_EXPR:
    return "plus";
  case MINUS_EXPR:
    return "minus";
  case MULT_EXPR:
    return "times";

  // divisions
  case TRUNC_DIV_EXPR:
    return "truncated_div";
  case CEIL_DIV_EXPR:
    return "ceiling_div";
  case FLOOR_DIV_EXPR:
    return "floor_div";
  case ROUND_DIV_EXPR:
    return "round_div";

  // mods
  case TRUNC_MOD_EXPR:
    return "truncated_mod";
  case CEIL_MOD_EXPR:
    return "ceiling_mod";
  case FLOOR_MOD_EXPR:
    return "floor_mod";
  case ROUND_MOD_EXPR:
    return "round_mod";

  case UNORDERED_EXPR:
    return "unordered";
  case ORDERED_EXPR:
    return "ordered";

  // real division (reals as in math)
  case RDIV_EXPR:
    return "real_div";

  // division with no rounding (point sub)
  case EXACT_DIV_EXPR:
    return "exact_div";

  // bit shifts and rotates
  case LSHIFT_EXPR:
    return "left_shift";
  case RSHIFT_EXPR:
    return "right_shift";
  case RROTATE_EXPR:
    return "rotate_right";
  case LROTATE_EXPR:
    return "rotate_left";

  // bit exprs
  case BIT_XOR_EXPR:
    return "bit_xor";
  case BIT_IOR_EXPR:
    return "bit_or";
  case BIT_AND_EXPR:
    return "bit_and";

  // logic exprs
  case GT_EXPR:
    return "bgt";
  case GE_EXPR:
    return "bgte";
  case LT_EXPR:
    return "blt";
  case LE_EXPR:
    return "blte";
  case NE_EXPR:
    return "bneq";
  case EQ_EXPR:
    return "beq";

  // unordered logic exprs
  case UNGT_EXPR:
    return "bungt";
  case UNGE_EXPR:
    return "bungte";
  case UNLT_EXPR:
    return "bunlt";
  case UNLE_EXPR:
    return "bunlte";
  case UNEQ_EXPR:
    return "buneq";

  // pointer exprs
  case POINTER_PLUS_EXPR:
    return "pointer_plus";

  // non-strict logical exprs
  case TRUTH_ANDIF_EXPR:
    return "truth_andif";
  case TRUTH_ORIF_EXPR:
    return "truth_orif";

  // strict logical exprs
  case TRUTH_AND_EXPR:
    return "band";
  case TRUTH_OR_EXPR:
    return "bor";
  case TRUTH_XOR_EXPR:
    return "bxor";

  // min and max
  case MIN_EXPR:
    return "minimum";
  case MAX_EXPR:
    return "maximum";

  default: {
    std::cerr << "Unrepresentable(\"Bad Op Code: " << get_tree_code_name(code) << "\"" << std::endl;
    assert(false);
  }
  }
}
}
}
} // c2ocaml::frontend::util
