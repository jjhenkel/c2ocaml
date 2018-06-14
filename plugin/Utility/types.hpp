/* types.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 6.10.2017
 * Description:
 *  - A bunch of nice typedefs and conversions
 *    for GCC's crazy structures
 */

#pragma once

namespace c2ocaml {
namespace frontend {
namespace types {

// Some basic typedefs that will be of use
typedef basic_block gcc_bb;
typedef tree gcc_block;
typedef tree gcc_type;
typedef tree gcc_tree;
typedef function *gcc_func;
typedef struct gimple *gcc_stmnt;
typedef enum gimple_code gcc_code;
typedef location_t gcc_local;
typedef edge gcc_edge;
typedef edge_iterator gcc_edge_iter;
typedef struct loop *gcc_loop;

typedef struct plugin_name_args *gcc_plugin_info;
typedef struct plugin_gcc_version *gcc_plugin_version;
typedef struct register_pass_info gcc_register_pass_info;
typedef pass_positioning_ops gcc_pass_positioning_ops;
typedef gcc::context *gcc_context;
typedef gimple_opt_pass gcc_gimple_pass;
typedef pass_data gcc_pass_data;

// Some more involved typdefs that represent usefull structures
typedef std::map<uint, std::pair<ulong, ulong>> bb_map;
typedef std::map<gcc_local, void (*)(gcc_stmnt &, gcc_code &, gcc_local &,
                                     gcc_bb &, gcc_block &)>
    gcc_stmt_switch;

// Conversion functions

inline auto as_a_gimple_call(struct gimple *statement) {
  return as_a<gcall *>(statement);
}

inline auto as_a_gimple_phi(struct gimple *statement) {
  return as_a<gphi *>(statement);
}

inline auto as_a_gimple_assign(struct gimple *statement) {
  return as_a<gassign *>(statement);
}

inline auto as_a_gimple_switch(struct gimple *statement) {
  return as_a<gswitch *>(statement);
}

inline auto as_a_gimple_asm(struct gimple *statement) {
  return as_a<gasm *>(statement);
}

inline auto as_a_gimple_conditional(struct gimple *statement) {
  return as_a<gcond *>(statement);
}

inline auto as_a_gimple_goto(struct gimple *statement) {
  return as_a<ggoto *>(statement);
}

inline auto as_a_gimple_debug(struct gimple *statement) {
  return as_a<gdebug *>(statement);
}

inline auto as_a_gimple_return(struct gimple *statement) {
  return as_a<greturn *>(statement);
}
}
}
} // c2ocaml::frontend::types
