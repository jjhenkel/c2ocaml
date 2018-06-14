/* utilities.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 6.10.2017
 * Description:
 *  - Pulls in a bunch of utility headers and
 *    provides any needed namespace aliases (for convenience)
 */

#pragma once

#define UNUSED(x) (void)(x)

#include "concat.hpp"
#include "constants.hpp"
#include "gcc-helpers.hpp"
#include "general-helpers.hpp"
#include "path-enumeration.hpp"
#include "types.hpp"

// Very heavily used utility func
using c2ocaml::frontend::util::gcc_str;
