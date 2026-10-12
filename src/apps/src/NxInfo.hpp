#pragma once
/**
 * @file NxInfo.hpp
 * @brief Declares reusable nxinfo command logic.
 */

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

/**
 * @namespace nx::core::nxinfo
 * @brief Contains the nxinfo command implementation.
 */
namespace nx::core::nxinfo
{
/**
 * @struct OutputStreams
 * @brief Groups the success and diagnostic streams to prevent argument-order mistakes.
 */
struct OutputStreams
{
  std::reference_wrapper<std::ostream> Output; ///< Receives successful command output.
  std::reference_wrapper<std::ostream> Error;  ///< Receives diagnostics and usage on errors.
};

/**
 * @brief Defines the plugin-loading dependency used before DREAM3D import.
 */
using LoadPluginsFunctionType = std::function<void(std::ostream&)>;

/**
 * @brief Runs nxinfo with the supplied command-line arguments.
 * @param arguments Command-line arguments excluding the executable name.
 * @param streams Success and diagnostic output streams.
 * @return Zero on success or an nxinfo error code.
 */
[[nodiscard]] int Run(const std::vector<std::string>& arguments, OutputStreams streams);

/**
 * @brief Runs nxinfo with an injected plugin-loading function.
 * @param arguments Command-line arguments excluding the executable name.
 * @param streams Success and diagnostic output streams.
 * @param loadPluginsFunction Function called before a valid DREAM3D input is read.
 * @return Zero on success or an nxinfo error code.
 * @pre loadPluginsFunction contains a callable target.
 */
[[nodiscard]] int Run(const std::vector<std::string>& arguments, OutputStreams streams, const LoadPluginsFunctionType& loadPluginsFunction);
} // namespace nx::core::nxinfo
