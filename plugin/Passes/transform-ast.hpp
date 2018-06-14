#include "../Utility/utilities.hpp"

namespace c2ocaml {
namespace frontend {
namespace v2 {

const char * LPAREN = "(";
const char * RPAREN = ")";
const char * EPREFIX = "Expr.";
const char * TPREFIX = "GccType.";
const char * COMMA = ", ";
const char * RAW_STR_OPEN = "\"";
const char * RAW_STR_CLOSE = "\"";
const char * UNSUPPORTED_EXPR = "unsupported";
const char * UNSUPPORTED_TYPE = "unrepresentable";
const char * START_LIST = "[|";
const char * END_LIST = "|]";
const char * LIST_SEP = ";";

// Use this to control if we output record details 
// (doing so can balloon the size of the generated sources 
//  but gives us more information)
const bool NO_INGEST_RECORD_DETAILS = true;

inline std::string transform_type(
  types::gcc_tree,
  std::map<void*, std::string>&, 
  std::stringstream&
);

inline std::string transform_ast(
  types::gcc_tree,
  std::map<void*, std::string>&, 
  std::map<void*, std::string>&, 
  std::stringstream&,
  std::stringstream&
);

inline std::string _transform_type(
  types::gcc_tree input, 
  std::map<void*, std::string>& TYPES_DONE, 
  std::stringstream& TYPES_BUFF
) {
  if (input == constants::nulltree) {
    return concatenate(TPREFIX, "none");
  }

  switch (TREE_CODE(input)) {
    case OFFSET_TYPE: {
      return concatenate(
        TPREFIX,
        "offset",
        LPAREN,
        transform_type(TYPE_OFFSET_BASETYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case ENUMERAL_TYPE: {
      return concatenate(
        TPREFIX, 
        UNSUPPORTED_TYPE, 
        LPAREN, 
        RAW_STR_OPEN,
        "ENUMERAL_TYPE",
        RAW_STR_CLOSE, 
        RPAREN
      );
    }
    case BOOLEAN_TYPE: {
      return concatenate(
        TPREFIX,
        "boolean"
      );
    }
    case INTEGER_TYPE: {
      return concatenate(
        TPREFIX,
        "integer",
        LPAREN,
        !TYPE_UNSIGNED(input) ? "true" : "false",
        COMMA,
        std::to_string(TYPE_PRECISION(input)),
        COMMA,
        gcc_str(TYPE_SIZE(input)),
        COMMA,
        "Z.of_string ",
        RAW_STR_OPEN,
        gcc_str(TYPE_MIN_VALUE(input)),
        RAW_STR_CLOSE,
        COMMA,
        "Z.of_string ",
        RAW_STR_OPEN,
        gcc_str(TYPE_MAX_VALUE(input)),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case REAL_TYPE: {
      return concatenate(
        TPREFIX, 
        "real", 
        LPAREN, 
        std::to_string(TYPE_PRECISION(input)), 
        RPAREN
      );
    }
    case POINTER_TYPE: {
      return concatenate(
        TPREFIX,
        "pointer",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case REFERENCE_TYPE: {
      return concatenate(
        TPREFIX,
        "reference",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case NULLPTR_TYPE: {
      return concatenate(
        TPREFIX,
        "nullptr"
      );
    }
    case FIXED_POINT_TYPE: {
      return concatenate(TPREFIX, UNSUPPORTED_TYPE, LPAREN, "FIXED_POINT_TYPE", RPAREN);
    }
    case COMPLEX_TYPE: {
      return concatenate(
        TPREFIX, 
        "complex", 
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case VECTOR_TYPE: {
      return concatenate(TPREFIX, UNSUPPORTED_TYPE, LPAREN, RAW_STR_OPEN, "VECTOR_TYPE", RAW_STR_CLOSE, RPAREN);
    }
    case ARRAY_TYPE: {
      return concatenate(
        TPREFIX,
        "array",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        TYPE_DOMAIN(input) ? transform_type(TYPE_DOMAIN(input), TYPES_DONE, TYPES_BUFF) : "GccType.none",
        RPAREN
      );
    }
    case UNION_TYPE:
    case RECORD_TYPE: {
      std::stringstream vars;
      std::stringstream fields;
      std::stringstream types;
      std::stringstream consts;

      vars << "        (* var decls *)" << std::endl;
      fields << "        (* field decls *)" << std::endl;
      types << "        (* type decls *)" << std::endl;
      consts << "        (* const decls *)" << std::endl;

      for (auto t = TYPE_FIELDS (input); t ; t = DECL_CHAIN (t)) {
        if (NO_INGEST_RECORD_DETAILS) {
          break;
        }

        if (TREE_CODE(t) == FIELD_DECL) {
          fields << concatenate(
            "        ",
            LPAREN, 
            "FieldDecl.make",
            LPAREN, 
            RAW_STR_OPEN,
            gcc_str(t),
            RAW_STR_CLOSE,
            COMMA,
            RAW_STR_OPEN,
            DECL_SIZE(t) ? gcc_str(DECL_SIZE(t)) : "0",
            RAW_STR_CLOSE,
            COMMA,
            std::to_string(DECL_ALIGN(t)),
            COMMA,
            RAW_STR_OPEN,
            DECL_FIELD_OFFSET(t) ? gcc_str(DECL_FIELD_OFFSET(t)) : "0",
            RAW_STR_CLOSE,
            COMMA,
            std::to_string(DECL_OFFSET_ALIGN(t)),
            COMMA,
            DECL_FIELD_BIT_OFFSET(t) ? gcc_str(DECL_FIELD_BIT_OFFSET(t)) : "0",
            COMMA,
            DECL_BIT_FIELD(t) ? "true" : "false",
            RPAREN,
            COMMA,
            transform_type(TREE_TYPE(t), TYPES_DONE, TYPES_BUFF),
            RPAREN,
            LIST_SEP             
          ) << std::endl;
        } else if (TREE_CODE(t) == TYPE_DECL) {
          types << concatenate(
            "        ",
            LPAREN, 
            RAW_STR_OPEN,
            gcc_str(t),
            RAW_STR_CLOSE,
            COMMA,
            transform_type(TREE_TYPE(t), TYPES_DONE, TYPES_BUFF),
            RPAREN,
            LIST_SEP
          ) << std::endl;
        } else if (TREE_CODE(t) == CONST_DECL) {
          consts << concatenate(
            "        ",
            LPAREN, 
            RAW_STR_OPEN,
            gcc_str(t),
            RAW_STR_CLOSE,
            COMMA,
            transform_type(TREE_TYPE(t), TYPES_DONE, TYPES_BUFF),
            COMMA,
            DECL_INITIAL(t) ? gcc_str(DECL_INITIAL(t)) : "",
            RPAREN,
            LIST_SEP
          ) << std::endl;
        } else if (TREE_CODE(t) == VAR_DECL) {
          vars << concatenate(
            "        ",
            LPAREN,
            "VarDecl.make",
            LPAREN, 
            RAW_STR_OPEN,
            gcc_str(t),
            RAW_STR_CLOSE,
            COMMA,
            DECL_SIZE(t) ? gcc_str(DECL_SIZE(t)) : "0",
            COMMA,
            std::to_string(DECL_ALIGN(t)),
            RPAREN,
            COMMA,
            transform_type(TREE_TYPE(t), TYPES_DONE, TYPES_BUFF),
            RPAREN,
            LIST_SEP
          ) << std::endl;
        }
      }

      return concatenate(
        TPREFIX, 
        TREE_CODE(input) == RECORD_TYPE ? "record" : "union", 
        LPAREN, 
        RAW_STR_OPEN, 
        gcc_str(TYPE_NAME(input)),
        RAW_STR_CLOSE, 
        COMMA,
        START_LIST,
        "\n",
        vars.str(),
        "      ",
        END_LIST,
        COMMA,
        START_LIST,
        "\n",
        fields.str(),
        "      ",
        END_LIST,
        COMMA,
        START_LIST,
        "\n",
        types.str(),
        "      ",
        END_LIST,
        COMMA,
        START_LIST,
        "\n",
        consts.str(),
        "      ",
        END_LIST,
        RPAREN
      );
    }
    case QUAL_UNION_TYPE: {
      return concatenate(
        TPREFIX, 
        UNSUPPORTED_TYPE, 
        LPAREN, 
        RAW_STR_OPEN, 
        "QUAL_UNION_TYPE::",
        gcc_str(input), 
        RAW_STR_CLOSE, 
        RPAREN
      );
    }
    case VOID_TYPE: {
      return concatenate(
        TPREFIX,
        "void"
      );
    }
    case POINTER_BOUNDS_TYPE: {
      return concatenate(TPREFIX, UNSUPPORTED_TYPE, LPAREN, "POINTER_BOUNDS_TYPE", RPAREN);
    }
    case FUNCTION_TYPE: {
      std::stringstream params;

      auto varargs = true; 

      for (auto t = TYPE_ARG_TYPES (input); t; t = TREE_CHAIN (t)) {
        if (t == void_list_node) {
          varargs = false;
          break;
        }
        
        params << concatenate(
          "        ",
          transform_type(TREE_VALUE(t), TYPES_DONE, TYPES_BUFF),
          LIST_SEP
        ) << std::endl;
      }

      return concatenate(
        TPREFIX, 
        "func", 
        LPAREN, 
        RAW_STR_OPEN,
        TYPE_NAME(input) ? gcc_str(TYPE_NAME(input)) :
          "T" + std::to_string(TYPE_UID(input)),
        RAW_STR_CLOSE,
        COMMA,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        START_LIST,
        "\n",
        params.str(),
        "      ",
        END_LIST,
        COMMA,
        varargs ? "true" : "false",
        RPAREN
      );
    }
    case METHOD_TYPE: {
      return concatenate(TPREFIX, UNSUPPORTED_TYPE, LPAREN, "METHOD_TYPE", RPAREN);
    }
    case LANG_TYPE: {
      return concatenate(
        TPREFIX,
        "UNSUPPORTED_TYPE",
        LPAREN,
        RAW_STR_OPEN,
        "LANG_TYPE::",
        gcc_str(input),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    default: {
      assert(false);
    }
  }
}

// TODO: work on this file

inline std::string _transform_ast(
  types::gcc_tree input,
  std::map<void*, std::string>& TYPES_DONE, 
  std::map<void*, std::string>& EXPRS_DONE, 
  std::stringstream& TYPES_BUFF,
  std::stringstream& EXPRS_BUFF
) {
  if (input == constants::nulltree) {
    return concatenate(EPREFIX, "nothing", LPAREN, "GccType.none", RPAREN);
  }

  switch (TREE_CODE(input)) {
    /* CONSTANTS */
    case VOID_CST: {
      return concatenate(
        EPREFIX, 
        "void_cst",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case INTEGER_CST: {
      auto asStr = gcc_str(input);
      util::str_numeric_only(asStr);
      if (TYPE_UNSIGNED(TREE_TYPE(input))) {
        return concatenate(
          EPREFIX, 
          "u_int_cst",
          LPAREN, 
          transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
          COMMA,
          "Z.of_string ",
          RAW_STR_OPEN,
          asStr,
          RAW_STR_CLOSE,
          RPAREN
        );
      } else {
        return concatenate(
          EPREFIX, 
          "s_int_cst",
          LPAREN, 
          transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
          COMMA,
          "Z.of_string ",
          RAW_STR_OPEN,
          asStr,
          RAW_STR_CLOSE,
          RPAREN
        );
      }
    }
    case REAL_CST: {
      return concatenate(
        EPREFIX, 
        "real_cst",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        gcc_str(input),
        RPAREN
      );
    }
    case FIXED_CST: {
      auto asStr = gcc_str(input);
      util::str_numeric_only(asStr);
      return concatenate(
        EPREFIX, 
        "FIXED_CST",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        asStr,
        RPAREN
      );
    }
    case COMPLEX_CST: {
      return concatenate(
        EPREFIX,
        "COMPLEX_CST",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_REALPART(input), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        COMMA,
        transform_ast(TREE_IMAGPART(input), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case VECTOR_CST: {
      return concatenate(
        EPREFIX, UNSUPPORTED_EXPR, LPAREN, "VECTOR_CST", RPAREN
      );
    }
    case STRING_CST: {
      std::string t;
      util::write_escaped(gcc_str(input), std::back_inserter(t));
      return concatenate(
        EPREFIX, 
        "string_cst", 
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        std::to_string(TREE_STRING_LENGTH(input)), 
        COMMA,
        t,
        RPAREN
      );
    }
    /* DECLARATIONS */
    case FUNCTION_DECL: {
      return concatenate(
        EPREFIX,
        UNSUPPORTED_EXPR,
        LPAREN,
        RAW_STR_OPEN,
        "FUNCTION_DECL",
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case LABEL_DECL: {
      return concatenate(
        EPREFIX,
        "label_decl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case RESULT_DECL: {
      return concatenate(
        EPREFIX,
        "resultdecl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case FIELD_DECL: {
      return concatenate(
        EPREFIX,
        "field_decl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        concatenate(
          "FieldDecl.make",
          LPAREN, 
          RAW_STR_OPEN,
          gcc_str(input),
          RAW_STR_CLOSE,
          COMMA,
          RAW_STR_OPEN,
          DECL_SIZE(input) ? gcc_str(DECL_SIZE(input)) : "0",
          RAW_STR_CLOSE,
          COMMA,
          std::to_string(DECL_ALIGN(input)),
          COMMA,
          RAW_STR_OPEN,
          DECL_FIELD_OFFSET(input) ? gcc_str(DECL_FIELD_OFFSET(input)) : "0",
          RAW_STR_CLOSE,
          COMMA,
          std::to_string(DECL_OFFSET_ALIGN(input)),
          COMMA,
          DECL_FIELD_BIT_OFFSET(input) ? gcc_str(DECL_FIELD_BIT_OFFSET(input)) : "0",
          COMMA,
          DECL_BIT_FIELD(input) ? "true" : "false",
          RPAREN
        ),
        RPAREN
      );
    }
    case VAR_DECL: {
      return concatenate(
        EPREFIX,
        "variable_decl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        concatenate(
          "VarDecl.make",
          LPAREN, 
          RAW_STR_OPEN,
          gcc_str(input),
          RAW_STR_CLOSE,
          COMMA,
          DECL_SIZE(input) ? gcc_str(DECL_SIZE(input)) : "0",
          COMMA,
          std::to_string(DECL_ALIGN(input)),
          RPAREN
        ),
        RPAREN
      );
    }
    case CONST_DECL: {
      return concatenate(
        EPREFIX,
        "const_decl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case PARM_DECL: {
      return concatenate(
        EPREFIX,
        "parameter_decl",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        COMMA,
        transform_type(DECL_ARG_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case TYPE_DECL: {
      return concatenate(
        EPREFIX,
        "TYPE_DECL",
        LPAREN, 
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    /* REFERENCES TO STORAGE */
    case COMPONENT_REF: {
      return concatenate(
        EPREFIX,
        "component_ref",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 1), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case BIT_FIELD_REF: {
      return concatenate(
        EPREFIX,
        "bitfield_ref",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        COMMA,
        TREE_OPERAND(input, 1) ? gcc_str(TREE_OPERAND(input, 1)) : "0",
        COMMA,
        TREE_OPERAND(input, 2) ? gcc_str(TREE_OPERAND(input, 2)) : "0",
        RPAREN
      );
    }
    case ARRAY_REF: {
      return concatenate(
        EPREFIX,
        "array_ref",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 1), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case MEM_REF: {
      return concatenate(
        EPREFIX,
        "memory_ref",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 1), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case REALPART_EXPR: {
      return concatenate(
        EPREFIX,
        "real_part",
        LPAREN, 
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case IMAGPART_EXPR: {
      return concatenate(
        EPREFIX,
        "imaginary_part",
        LPAREN, 
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case ADDR_EXPR: {
      return concatenate(
        EPREFIX,
        "address_of",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        COMMA,
        transform_ast(TREE_OPERAND(input, 0), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF),
        RPAREN
      );
    }
    case VIEW_CONVERT_EXPR: {
      return concatenate(
        EPREFIX,
        UNSUPPORTED_EXPR,
        LPAREN,
        RAW_STR_OPEN,
        "VIEW_CONVERT_EXPR",
        RAW_STR_CLOSE,
        RPAREN
      );
    }
    case CONSTRUCTOR: {
      return concatenate(
        EPREFIX,
        "constructor",
        LPAREN,
        transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
        RPAREN
      );
    }
    case SSA_NAME: {
      return concatenate(
        EPREFIX,
        "ssa",
        LPAREN, 
        RAW_STR_OPEN,
        gcc_str(input),
        RAW_STR_CLOSE,
        COMMA,
        std::to_string(SSA_NAME_VERSION(input)),
        COMMA,
        // Either there is more to this definition, or it is something like 
        // a temporary waiting to be filled during execution (and so we grab 
        // the type right away)
        SSA_NAME_VAR(input) ? transform_ast(SSA_NAME_VAR(input), TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF) : 
          concatenate(
            EPREFIX, 
            "nothing", 
            LPAREN, 
            transform_type(TREE_TYPE(input), TYPES_DONE, TYPES_BUFF),
            RPAREN
          ),
        RPAREN
      );
    }
    default: {
      std::cout << get_tree_code_name(TREE_CODE(input)) << std::endl;
      assert(false);
    }
  }
}

inline std::string transform_type(
  types::gcc_tree input, 
  std::map<void*, std::string>& TYPES_DONE, 
  std::stringstream& TYPES_BUFF
) {
  if (TYPES_DONE.find(input) == TYPES_DONE.end()) {
      std::ostringstream tmp, tmp2;

      tmp << (void const *)input;
      TYPES_DONE[input] = "_typeSELF";

      tmp2 << _transform_type(input, TYPES_DONE, TYPES_BUFF);
      TYPES_BUFF << "  in let type" << tmp.str() << " = " << std::endl 
                 << "    " << tmp2.str() << std::endl;

      TYPES_DONE[input] = "type" + tmp.str();
  }

  return TYPES_DONE[input];
}

inline std::string transform_ast(
  types::gcc_tree input,
  std::map<void*, std::string>& TYPES_DONE, 
  std::map<void*, std::string>& EXPRS_DONE, 
  std::stringstream& TYPES_BUFF,
  std::stringstream& EXPRS_BUFF
) {
  if (EXPRS_DONE.find(input) == EXPRS_DONE.end()) {
      std::ostringstream tmp, tmp2;
      tmp << (void const *)input;
      tmp2 << _transform_ast(
        input, TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF
      );
      EXPRS_BUFF << "  in let expr" << tmp.str() << " = " << std::endl 
                 << "    " << tmp2.str() << std::endl;
      EXPRS_DONE[input] = "expr" + tmp.str();
  }

  return EXPRS_DONE[input];
}

} // namespace v2
} // namespace frontend
} // namespace c2ocaml
