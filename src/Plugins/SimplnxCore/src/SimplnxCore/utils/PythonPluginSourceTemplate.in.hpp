#pragma once

/**
 * This file is auto-generated during CMake time. Any changes made to the generated file
 * will be over written the next time CMake runs a configure.
 */

#include <string>

namespace nx::core
{

// clang-format off
static const char k_PythonFilterTemplateCharArray[] = {@PYTHON_FILTER_TEMPLATE@};
// clang-format on

inline const std::string PythonFilterTemplate()
{
  return {k_PythonFilterTemplateCharArray};
}

// clang-format off
static const char k_PluginPythonFileCharArray[] = {@PYTHON_PLUGIN_TEMPLATE@};
// clang-format on

inline const std::string PluginPythonFile()
{
  return {k_PluginPythonFileCharArray};
}

// clang-format off
static const char k_PluginInitPythonFileCharArray[] = {@PYTHON_PLUGIN_INIT_TEMPLATE@};
// clang-format on

inline const std::string PluginInitPythonFile()
{
  return {k_PluginInitPythonFileCharArray};
}

// clang-format off
static const char k_PluginEnvironmentPythonFileCharArray[] = {@PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE@};
// clang-format on

inline const std::string PluginEnvironmentPythonFile()
{
  return {k_PluginEnvironmentPythonFileCharArray};
}

}; // namespace nx::core