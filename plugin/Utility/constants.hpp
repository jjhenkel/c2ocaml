/* util/constants.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 6.10.2017
 * Description:
 *  - GCC Constants
 */

#pragma once

#include "types.hpp"

namespace c2ocaml {
namespace frontend {
namespace constants {

const char *GCC_SSA_PASS = "ssa";
const char *GCC_CFG_PASS = "cfg";

const int32_t GCC_PLUGIN_SUCCESS = 0;
const uint32_t GCC_EXECUTE_SUCCESS = 0;

const types::gcc_tree nulltree = 0;
}
}
} // c2ocaml::frontend::constants
