#ifndef c2ocaml_PCH_HPP_
#define c2ocaml_PCH_HPP_

// standard includes
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

#include <sys/stat.h>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;

#include <linux/limits.h>
#include <stdio.h>

#include "assert.h"

#include <gmpxx.h>

// Have to include this one first
#include "gcc-plugin.h"

// Defines GCC version macros
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "plugin-version.h"
#pragma GCC diagnostic pop

// General gcc plugin utilities
#include "basic-block.h"
#include "context.h"
#include "coretypes.h"
#include "dumpfile.h" /* for dump_flags */
#include "function.h"
#include "internal-fn.h"
#include "is-a.h"
#include "predict.h"
#include "stor-layout.h"
#include "trans-mem.h"
#include "tree-dump.h"
#include "tree-eh.h"
#include "tree-pass.h"
#include "tree-pretty-print.h"
#include "tree-ssa-alias.h"
#include "tree.h"
#include "tree-cfg.h"
#include "value-prof.h"

// For gimple
#include "gimple.h"

#include "gimple-expr.h"
#include "gimple-iterator.h"
#include "gimple-predict.h"
#include "gimple-pretty-print.h"
#include "gimple-walk.h"

// SSA & cgraph utils
#include "cfganal.h"
#include "cfgloop.h"
#include "cgraph.h"
#include "ssa.h"

#endif // c2ocaml_PCH_HPP_
