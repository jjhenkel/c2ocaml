/* uw-pass.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 6.10.2017
 * Description:
 *  - Base class for our custom gcc passes
 */

#pragma once

#include "../Utility/utilities.hpp"

namespace c2ocaml {
namespace frontend {
namespace common {

using types::gcc_pass_data;
using types::gcc_gimple_pass;
using types::gcc_plugin_info;
using types::gcc_plugin_version;
using types::gcc_register_pass_info;
using types::gcc_pass_positioning_ops;

/*
 * uw_pass - abstract base + static utility functions for registering
 *           our own custom gcc passes that have startup/shutdwon
 *           functionality
 */
struct uw_pass : public gcc_gimple_pass {
protected:
  std::string project;
  gcc_plugin_info plugin_info;
  gcc_plugin_version version;

public:
  uw_pass(gcc_pass_data dat, gcc_plugin_info pi, gcc_plugin_version vr,
          const std::string &proj)
      : gcc_gimple_pass(dat, g) {
    project = proj;
    plugin_info = pi;
    version = vr;
  }

  virtual bool init() = 0;
  virtual void deinit() = 0;

  inline static void init(void *gcc_data, void *user_data) {
    UNUSED(gcc_data);
    ((uw_pass *)user_data)->init();
  }

  inline static void deinit(void *gcc_data, void *user_data) {
    UNUSED(gcc_data);
    ((uw_pass *)user_data)->deinit();
  }

  template <typename pass>
  inline static void register_pass_after(gcc_plugin_info plugin_info,
                                         gcc_plugin_version plugin_version,
                                         const std::string &project,
                                         const std::string &after) {
    uw_pass::register_pass<pass>(plugin_info, plugin_version, project, after,
                                 PASS_POS_INSERT_AFTER);
  }

  template <typename pass>
  inline static void register_pass_before(gcc_plugin_info plugin_info,
                                          gcc_plugin_version plugin_version,
                                          const std::string &project,
                                          const std::string &before) {
    uw_pass::register_pass<pass>(plugin_info, plugin_version, project, before,
                                 PASS_POS_INSERT_BEFORE);
  }

  template <typename pass>
  inline static void register_pass_replace(gcc_plugin_info plugin_info,
                                           gcc_plugin_version plugin_version,
                                           const std::string &project,
                                           const std::string &replace) {
    uw_pass::register_pass<pass>(plugin_info, plugin_version, project, replace,
                                 PASS_POS_REPLACE);
  }

  template <typename pass>
  inline static void
  register_pass(gcc_plugin_info plugin_info, gcc_plugin_version plugin_version,
                const std::string &project, const std::string &target,
                gcc_pass_positioning_ops position) {
    gcc_register_pass_info pass_info;

    auto our_pass = new pass(plugin_info, plugin_version, project);

    pass_info.pass = our_pass;
    pass_info.reference_pass_name = target.c_str();
    pass_info.ref_pass_instance_number = 0;
    pass_info.pos_op = position;

    register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                      &pass_info);

    register_callback(plugin_info->base_name, PLUGIN_FINISH, &uw_pass::deinit,
                      pass_info.pass);
  }
};
}
}
} // c2ocaml::frontend::common
