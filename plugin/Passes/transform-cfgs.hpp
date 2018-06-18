/* transform-cfgs.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 9.11.2017
 * Description:
 *  - The custom compiler pass that walks over control flow graphs (CFGs)
 *    and creates ...
 */

#pragma once

#include "../Utility/utilities.hpp"
#include "transform-ast.hpp"

#define OUTP outt

namespace c2ocaml {
namespace frontend {
namespace passes {

using types::gcc_func;
using types::gcc_context;
using types::gcc_pass_data;
using types::gcc_plugin_info;
using types::gcc_plugin_version;

/*
 * Data that defines our pass for GCC (a description of the pass
 * and what it requires and how it should behave... but NOT the
 * registration of the pass)
 */
const gcc_pass_data deposit_ecfgs_pass_data = {GIMPLE_PASS,
                                               "transform_cfgs_pass",
                                               OPTGROUP_NONE,
                                               TV_NONE,
                                               PROP_gimple_any,
                                               0,
                                               0,
                                               0,
                                               0}; // end pass_data

/*
 * transform_ecfgs - the main pass of our frontend. See beginning of file for
                   more details.
 */
class transform_cfgs : public common::uw_pass {
private:

public:
  transform_cfgs(gcc_plugin_info info, gcc_plugin_version ver,
                 const std::string &proj)
      : common::uw_pass(deposit_ecfgs_pass_data, info, ver, proj) {

    
  }

  ~transform_cfgs() {}

  inline virtual uint32_t execute(gcc_func procedure) override {
    std::stringstream outt, outs, callstr, TYPES_BUFF, EXPRS_BUFF;
    std::map<void*, std::string> TYPES_DONE, EXPRS_DONE;

    auto transform_ast = [&](types::gcc_tree input, std::ostream & out) {
      out << v2::transform_ast(input, TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF);
    };

    // auto transform_ast2 = [&](types::gcc_tree input, std::stringstream& out) {
    //   out << v2::transform_ast(input, TYPES_DONE, EXPRS_DONE, TYPES_BUFF, EXPRS_BUFF);
    // };

    if (procedure->decl == nullptr) {
      OUTP << "WARN: ignored procedure with no decl" << std::endl;
      return constants::GCC_EXECUTE_SUCCESS;
    }

    // We DO NOT want to fool around with functions we don't really
    // have the source for
    if (DECL_EXTERNAL(procedure->decl)) {
      return constants::GCC_EXECUTE_SUCCESS;
    }

    auto as_decl = procedure->decl;
    auto name = gcc_str(DECL_NAME(as_decl));

    // Ignore junk related to gcov
    if (name.find("_GLOBAL__sub_I_00100") != std::string::npos || 
        name.find("_GLOBAL__sub_D_00100") != std::string::npos) {
      return constants::GCC_EXECUTE_SUCCESS;
    }
    
    if (name.find("_GLOBAL__sub_I_65535") != std::string::npos ||
        name.find("_GLOBAL__sub_D_65535") != std::string::npos) {
      return constants::GCC_EXECUTE_SUCCESS;
    }

    auto source_file_name = std::string(
        (DECL_SOURCE_FILE(as_decl) ? DECL_SOURCE_FILE(as_decl) : "unknown"));

    // Skip these kind of files 
    if (source_file_name == "unknown" || source_file_name == "<stdin>") {
      return constants::GCC_EXECUTE_SUCCESS;
    }

    fs::path fp = "/common/facts";

    auto repocwd = util::repo_cwd();
    util::str_replace(repocwd, "/target/" + project, "");
    
    auto tempName = project 
      + repocwd 
      + "/"
      + std::string(main_input_basename) 
      + "." 
      + std::to_string(procedure->funcdef_no);
    
    util::str_replace_all(tempName, "/", "_");

    fp /= source_file_name;
    fp /= name;
    fp += ".ml";

    // Collapse /../ --> /
    auto helper = fp.string();
    util::str_replace_all(helper, "/../", "/");
    fp = helper;

    if (util::fexists(fp.string())) {
      std::cerr << fp.string() + " exists... skipping.\n";
      return constants::GCC_EXECUTE_SUCCESS;
    }

    std::cerr << "Created: " + fp.string() + "\n";

    fs::create_directories(fp.parent_path());

    outs << std::endl;
    outs << "let main = " << std::endl;
   
    outs << "(*-------------------------------------------------------- " << std::endl;
    outs << "  // working_directory: " << util::repo_cwd() << std::endl;
    outs << "  // source_file_name: " << source_file_name << std::endl;
    outs << "  // base_name: " << main_input_basename << std::endl;
    outs << "  // name: " << name << std::endl;
    outs << "  // fid: " << procedure->funcdef_no << std::endl;
    outs << "  ---------------------------------------------------------*)" << std::endl;

    outs << std::endl;

    // NEED TO DO ONE PRE-PASS FOR PHI/IF/SWITCH 
    std::map<uint32_t, std::vector<
      std::pair<std::string, std::string> > > blocksToLines;
    std::map<uint32_t, std::vector<
      std::pair<std::string, std::string> > > blocksToLinesEnd;

    util::for_each_bb(procedure, [&](types::gcc_bb bb, int32_t index) {
      util::for_each_stmt(bb, [&](auto gs) {
        if (gimple_code(gs) == GIMPLE_COND) {
          auto input = as_a<gcond *>(gs);
          int32_t trueBlockIndex = -1;
          int32_t falseBlockIndex = -1;

          util::for_each_bb_succ(bb, [&](types::gcc_edge edge) {
            if (edge->flags & EDGE_TRUE_VALUE) {
              trueBlockIndex = edge->dest->index;
            } else if (edge->flags & EDGE_FALSE_VALUE) {
              falseBlockIndex = edge->dest->index;
            }
          });

          assert(trueBlockIndex != -1);
          assert(falseBlockIndex != -1);

          std::stringstream trueSS, falseSS;
          trueSS << "      Action.assume(" << index << ", Expr." <<
            util::gcc_str_code(gimple_cond_code(input)) << "(GccType.boolean, ";
          transform_ast(gimple_cond_lhs(input), trueSS);
          trueSS << ", ";
          transform_ast(gimple_cond_rhs(input), trueSS);
          trueSS << "))";
            
          falseSS << "      Action.assume(" << index << ", Expr." <<
            util::gcc_str_code_inverse(gimple_cond_code(input)) << "(GccType.boolean, ";
          transform_ast(gimple_cond_lhs(input), falseSS);
          falseSS << ", ";
          transform_ast(gimple_cond_rhs(input), falseSS);
          falseSS << "))";
              
          auto asastr = gcc_str(input);
          util::str_replace(asastr, "if ", ""); 

          blocksToLines[trueBlockIndex].push_back(
            std::pair<std::string, std::string>(
              trueSS.str(), 
              "assume TRUE " + asastr
            ));
          blocksToLines[falseBlockIndex].push_back(
            std::pair<std::string, std::string>(
              falseSS.str(),
              "assume FALSE " + asastr
            ));
        } else if (gimple_code(gs) == GIMPLE_PHI) {
          auto input = as_a<gphi *>(gs);
          auto argCount = gimple_phi_num_args(input);
          
          // Ignore .MEM phi assignments
          if (virtual_operand_p(gimple_phi_result(input))) {
            return;
          }
          
          for (uint32_t i = 0; i < argCount; i++) {
            std::stringstream tempSS;
            auto targetIndex = gimple_phi_arg_edge(input, i)->src->index;
         
            tempSS << "      Action.assign(";
            transform_ast(gimple_phi_result(input), tempSS); 
            tempSS << ", ";
            transform_ast(PHI_ARG_DEF(input, i), tempSS);
            tempSS << ")";

            blocksToLinesEnd[targetIndex].push_back(
              std::pair<std::string, std::string>(
                tempSS.str(), 
                gcc_str(gimple_phi_result(input)) + " = " + gcc_str(PHI_ARG_DEF(input, i))
              ));
          }
        } else if (gimple_code(gs) == GIMPLE_SWITCH) {
          auto input = as_a<gswitch *>(gs);
          auto argCount = gimple_switch_num_labels(input);

          std::string endp = "";
          std::stringstream defaultSS, defaultDBG;
          uint32_t defaultBlockIndex = 0;

          defaultSS << "      Action.assume(" << index << ", Expr.bnot(GccType.boolean,";
          defaultDBG << "assume FALSE ((";
          bool wroteone = false;

          for (uint32_t i = 0; i < argCount; ++i) {
            std::stringstream tempSS, tempDBG;
            auto label = gimple_switch_label(input, i);
            auto targetIndex = label_to_block(CASE_LABEL(label))->index;
            
            bool noLow = false, noHigh = false;

            tempDBG << "assume TRUE (";

            if (!CASE_LOW(label)) {
              noLow = true;
            }

            if (!CASE_HIGH(label)) {
              noHigh = true;
            }

            if (noLow && noHigh) {
              defaultBlockIndex = targetIndex;
              continue;
            } else if (noHigh) {
              tempSS << "      Action.assume(" << index << ", Expr.beq(GccType.boolean, "; 
              transform_ast(gimple_switch_index(input), tempSS);
              tempSS << ", ";
              transform_ast(CASE_LOW(label), tempSS);
              tempSS << "))";

              tempDBG << gcc_str(gimple_switch_index(input)) 
                      << " == " << gcc_str(CASE_LOW(label)) << ")";

              if (i < argCount - 1) {
                defaultSS << " Expr.bor(GccType.boolean, Expr.beq(GccType.boolean, ";
              } else {
                defaultSS << " Expr.beq(GccType.boolean, ";
              }

              if (wroteone) {
                defaultDBG << " or (";
              }

              transform_ast(gimple_switch_index(input), defaultSS);
              defaultSS << ", ";
              transform_ast(CASE_LOW(label), defaultSS);
              defaultSS << ")";

              defaultDBG << gcc_str(gimple_switch_index(input)) 
                         << " == " << gcc_str(CASE_LOW(label)) << ")";
            } else {
              tempSS << "      Action.assume(" << index << ", Expr.inrange(GccType.boolean, "; 
              transform_ast(gimple_switch_index(input), tempSS);
              tempSS << ",";
              transform_ast(CASE_LOW(label), tempSS);
              tempSS << ",";
              transform_ast(CASE_HIGH(label), tempSS);
              tempSS << "))";
              
              tempDBG << gcc_str(gimple_switch_index(input)) 
                      << " >= " << gcc_str(CASE_LOW(label))
                      << " and " << gcc_str(gimple_switch_index(input)) 
                      << " <= " << gcc_str(CASE_HIGH(label)) << ")";

              if (i < argCount - 1) {
                defaultSS << " Expr.bor(GccType.boolean, Expr.inrange(GccType.boolean, ";
              } else {
                defaultSS << " Expr.inrange(GccType.boolean, ";
              }

              if (wroteone) {
                defaultDBG << " or (";
              }

              transform_ast(gimple_switch_index(input), defaultSS);
              defaultSS << ", "; 
              transform_ast(CASE_LOW(label), defaultSS);
              defaultSS << ", "; 
              transform_ast(CASE_HIGH(label), defaultSS);
              defaultSS << ")";

              defaultDBG << gcc_str(gimple_switch_index(input)) 
                         << " >= " << gcc_str(CASE_LOW(label))
                         << " and " << gcc_str(gimple_switch_index(input)) 
                         << " <= " << gcc_str(CASE_HIGH(label)) << ")";
            }

            if (i < argCount - 1) {
              endp += ")";
              defaultSS << ",";
            }
            
            wroteone = true;

            blocksToLines[targetIndex].push_back(
              std::pair<std::string, std::string>(
                tempSS.str(), tempDBG.str()
              ));
          }

          defaultSS << endp << "))";
          defaultDBG << ")";

          blocksToLines[defaultBlockIndex].push_back(
            std::pair<std::string, std::string>(defaultSS.str(), defaultDBG.str())
          );
        }
      });
    });

    size_t sIndex = -1;
    util::for_each_bb(procedure, [&](types::gcc_bb bb, int32_t index) {
      std::vector<std::string> calls;

      OUTP << "  in let block_" << index << " = " << std::endl;

      if (index == 0) {
        OUTP << "    let step_0_" << ++sIndex << " = Action.start " << std::endl;
        OUTP << "    in Block.block (" << std::endl;
        OUTP << "      " << index << "," << std::endl;
        OUTP << "      [| step_0_" << sIndex << " |]," << std::endl;
        OUTP << "      [||]," << std::endl;
        OUTP << "      [| \"<ENTRY>\" |]" << std::endl;
        OUTP << "    )" << std::endl;
        return;
      } else if (index == 1) {
        OUTP << "    let step_0_" << ++sIndex << " = Action.finish " << std::endl;
        OUTP << "    in Block.block (" << std::endl;
        OUTP << "      " << index << "," << std::endl;
        OUTP << "      [| step_0_" << sIndex << " |]," << std::endl;
        OUTP << "      [||]," << std::endl;
        OUTP << "      [| \"<EXIT>\" |]" << std::endl;
        OUTP << "    )" << std::endl;
        return;
      }

      bool firsts = true;
      std::vector<std::string> stmtstrs, dbgstrs;

      for (auto & line : blocksToLines[index]) {
        if (firsts) {
          OUTP << "    ";
          firsts = false;        
        } else {
          OUTP << "    in ";
        }

        sIndex += 1;
        OUTP << "let step_" << index << "_" << sIndex << " = " << std::endl << line.first << std::endl;
        stmtstrs.push_back("step_" + std::to_string(index) + "_" + std::to_string(sIndex));
        dbgstrs.push_back(line.second);
      }

      util::for_each_stmt(bb, [&](auto gs) {
        if (gimple_code(gs) == GIMPLE_COND || 
            gimple_code(gs) == GIMPLE_PHI || 
            gimple_code(gs) == GIMPLE_SWITCH) {
          return; // Handled in pre/post
        }

        if (firsts) {
          OUTP << "    ";
          firsts = false;        
        } else {
          OUTP << "    in ";
        }

        sIndex += 1;
        OUTP << "let step_" << index << "_" << sIndex << " = " << std::endl;
        stmtstrs.push_back("step_" + std::to_string(index) + "_" + std::to_string(sIndex));
        
        dbgstrs.push_back(gcc_str(gs));

        switch (gimple_code(gs)) {
        case GIMPLE_ASM: {
          std::string t; 
          util::write_escaped(gcc_str(gs), std::back_inserter(t));
          OUTP << "      Action.unsupport(" << t << ")";
          break;
        }
        case GIMPLE_ASSIGN: {
          auto input = as_a<gassign *>(gs);
          switch (gimple_num_ops(input) - 1) {
          case 1: {
            OUTP << "      Action.assign(";
            transform_ast(gimple_assign_lhs(input), OUTP);
            OUTP << ", ";
            transform_ast(gimple_assign_rhs1(input), OUTP);
            // TODO: model gimple_assign_cast_p(input);
            OUTP << ")";
            break;
          }
          // X = Y <op> Z
          case 2: {
            OUTP << "      Action.assign(";
            transform_ast(gimple_assign_lhs(input), OUTP);
            OUTP << ", Expr." << util::gcc_str_code(gimple_cond_code(input)) << "(" << std::endl;
            OUTP << "        " << v2::transform_type(
              TREE_TYPE(gimple_assign_lhs(input)), TYPES_DONE, TYPES_BUFF) << ", ";
            transform_ast(gimple_assign_rhs1(input), OUTP);
            OUTP << ", ";
            transform_ast(gimple_assign_rhs2(input), OUTP);
            OUTP << "))";
            break;
          }
          case 3: {
            OUTP << "      Action.unsupport(\"" << gcc_str(gs) << "\")";
            break;
          }
          default: {
            assert(false);
          }
          }
          break;
        }
        case GIMPLE_CALL: {
          auto input = as_a<gcall *>(gs);
          auto callName = std::string("???");

          if (gimple_call_fndecl(input)) {
            callName = std::string(gcc_str(gimple_call_fndecl(input)));
          } else {
            OUTP << "      Action.unsupport(\"Called function pointer?\")";
            break;
            // callName = std::string(gcc_str(gimple_call_fn(input)));
          }

          auto capturesReturn = (gimple_call_lhs(input) != constants::nulltree);

          if (callName.size() == 0) {
            OUTP << "      Action.unsupport(\"VA_ARG stuff?\")";
            break;
          }

          calls.push_back(callName);

          std::vector<std::string> argNames;

          if (gimple_call_fndecl(input)) {
            uint32_t i = 0;
            for (auto arg = DECL_ARGUMENTS(gimple_call_fndecl(input)); arg;
                 arg = DECL_CHAIN(arg), ++i) {
              argNames.push_back(gcc_str(TREE_VALUE(arg)));
            }
          }

          callstr << "  in let call" << input << " = " << "Expr.call(" << std::endl;
          callstr << "    " << v2::transform_type(gimple_expr_type(input), TYPES_DONE, TYPES_BUFF) << "," << std::endl;
          callstr << "    \"" << callName << "\", [|" << std::endl;  

          for (uint32_t i = 0; i < gimple_call_num_args(input); i++) {
            if (i >= argNames.size()) {
              argNames.push_back("p" + std::to_string(i+1));
            }

            callstr << "      (Expr.parameter(\""
                << argNames[i] << "\", " << i << ", "; 
            transform_ast(gimple_call_arg(input, i), callstr);
            callstr << "));" << std::endl;
          }

          callstr << "  |])" << std::endl;

          if (capturesReturn) {
            OUTP << "      Action.call(call" << input << ")" << std::endl;
            sIndex += 1;
            OUTP << "    in let step_" << index << "_" << sIndex << " = " << std::endl;
            stmtstrs.push_back("step_" + std::to_string(index) + "_" + std::to_string(sIndex));
            dbgstrs.push_back("<CAPTURES RETURN>");
            OUTP << "      Action.assign(";
            transform_ast(gimple_call_lhs(input), OUTP);
            OUTP << ", call" << input << ")";
          } else {
            OUTP << "      Action.call(call" << input << ")";
          }

          break;
        }
        case GIMPLE_DEBUG: {
          OUTP << "      Action.debug(\"" << gcc_str(gs) << "\")";
          break;
        }
        case GIMPLE_GOTO: {
          OUTP << "      Action.nop (* was goto *)";
          break;
        }
        case GIMPLE_LABEL: {
          auto input = as_a<glabel *>(gs);
          OUTP << "      Action.observe(";
          transform_ast(gimple_label_label(input), OUTP);
          OUTP << ")";
          break;
        }
        case GIMPLE_NOP: {
          OUTP << "      Action.nop" << std::endl;
          break;
        }
        case GIMPLE_PREDICT: {
          OUTP << "      Action.predict(\"" << gcc_str(gs) << "\")";
          break;
        }
        case GIMPLE_RETURN: {
          auto input = as_a<greturn *>(gs);
          OUTP << "      Action.return(";
          transform_ast(gimple_return_retval(input), OUTP);
          OUTP << ")";
          break;
        }
        case GIMPLE_RESX: {
          OUTP << "      Action.unsupport(\"" << gcc_str(gs) << "\")";
          break;
        }
        case GIMPLE_EH_DISPATCH: {
          OUTP << "      Action.unsupport(\"" << gcc_str(gs) << "\")";
          break;
        }
        default: {
          assert(false);
        }
        }
        OUTP << std::endl;
      });
      
      for (auto & line : blocksToLinesEnd[index]) {
        if (firsts) {
          OUTP << "    ";
          firsts = false;        
        } else {
          OUTP << "    in ";
        }

        sIndex += 1;
        OUTP << "let step_" << index << "_" << sIndex << " = " << std::endl << line.first << std::endl;
        stmtstrs.push_back("step_" + std::to_string(index) + "_" + std::to_string(sIndex));
        dbgstrs.push_back(line.second);
      }

      if (firsts) {
        OUTP << "    ";
      } else {
        OUTP << "    in ";
      }

      std::map<std::string, int> cmap;

      for (auto & c : calls) {
        if (cmap.find(c) == cmap.end()) {
          cmap[c] = 0;
        }
        cmap[c] += 1;
      }

      OUTP << "Block.block(" << std::endl;
      OUTP << "      " << index << "," << std::endl;
      OUTP << "      [|" << std::endl;
      for (auto & s : stmtstrs) {
        OUTP << "        " << s << ";" << std::endl;
      }
      OUTP << "      |]," << std::endl;
      OUTP << "      [|" << std::endl;
      auto anyc = false;
      for (auto & c : cmap) {
        OUTP << "        (\"" << c.first << "\", " << c.second << ");" << std::endl;
        anyc = true;
      }
      if (!anyc) {
        OUTP << "        (* no calls *)" << std::endl;
      }
      OUTP << "      |]," << std::endl;
      OUTP << "      [|" << std::endl;
      for (auto & s : dbgstrs) {
        std::string t;
        util::write_escaped(s, std::back_inserter(t));
        OUTP << "        " << t << ";" << std::endl;
      }
      OUTP << "      |]" << std::endl;
      OUTP << "    )" << std::endl;

    });

    outs << "  let _typeSELF = GccType.pointer(GccType.self)" << std::endl;
    outs << TYPES_BUFF.str();
    outs << EXPRS_BUFF.str();
    outs << callstr.str();

    OUTP << PathEnumerator::Enumerate(procedure) << std::endl;

    OUTP << "  in Proc.proc(" << std::endl;
    OUTP << "   \"" << name << "\"," << std::endl;
    OUTP << "    " << procedure->funcdef_no << "," << std::endl;
    OUTP << "   \"" << util::repo_cwd() << "\"," << std::endl;
    OUTP << "   \"" << source_file_name << "\"," << std::endl;
    OUTP << "   \"" << main_input_basename << "\"," << std::endl;
    OUTP << "    cfg" << std::endl;
    OUTP << "  )" << std::endl;
    OUTP << "in Driver.execute main;;";

    outs << OUTP.str() << std::endl;

    std::ofstream outf;
    outf.open(fp.string(), std::ofstream::out | std::ofstream::trunc);
    
    outf << outs.str() << std::endl;
    
    outf.flush();
    outf.close();

    return constants::GCC_EXECUTE_SUCCESS;
  }

  inline transform_cfgs *clone() override { return this; }

  inline bool init() override { return true; }

  inline void deinit() override {

  }
};
}
}
} // c2ocaml::frontend::passes
