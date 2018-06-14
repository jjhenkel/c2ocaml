#include "Common/pch.hpp"
#include "Common/uw-pass.hpp"

#include "Passes/transform-cfgs.hpp"

#include "Utility/utilities.hpp"

// GCC requires this
int plugin_is_GPL_compatible;

using namespace c2ocaml::frontend;

int plugin_init(types::gcc_plugin_info plugin_info,
                types::gcc_plugin_version plugin_version) {

  // Check for project arg
  if (plugin_info->argc <= 0 ||
      strcmp(plugin_info->argv[0].key, "project") != 0) {
    std::cerr << "WARN: Flag -fplugin-arg-project= is required" << std::endl;
    std::cerr << "WARN: Exiting." << std::endl;

    // We actually will end gracefully here still
    // some frameworks do compilation sanity checks
    // and we wont ingest those source (no project name
    // would be specified, but we still need to exit
    // gracefully)
    return constants::GCC_PLUGIN_SUCCESS;
  }

  auto project = std::string(plugin_info->argv[0].value);

  // Attach our pass to extract the ECFGs after they
  // become avaliable (anytime after GCC's cfg pass)
  common::uw_pass::register_pass_after<passes::transform_cfgs>(
      plugin_info, plugin_version, project, constants::GCC_SSA_PASS);

  // Return success
  return constants::GCC_PLUGIN_SUCCESS;
}
