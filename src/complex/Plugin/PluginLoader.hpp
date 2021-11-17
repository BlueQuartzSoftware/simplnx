#pragma once

#include <filesystem>
#include <memory>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class PluginLoader
 * @brief The PluginLoader class is the control wrapper around loading,
 * unloading, and accessing plugins. The implementation of PluginLoader is
 * provided for each operating system, but the public API remains the same.
 */
class COMPLEX_EXPORT PluginLoader
{
public:
  /**
   * @brief Constructs a PluginLoader targetting the specified path.
   * The plugin is loaded upon construction and unloaded when the object is
   * destroyed.
   * @param path
   */
  PluginLoader(const std::filesystem::path& path);

  ~PluginLoader() noexcept;

  /**
   * @brief Returns true if the plugin is loaded. Returns false otherwise.
   * @return bool
   */
  bool isLoaded() const;

  /**
   * @brief Returns a pointer to the loaded plugin.
   * @return AbstractPlugin*
   */
  AbstractPlugin* getPlugin() const;

private:
  /**
   * @brief Loads the plugin using the target platform's API.
   */
  void loadPlugin();

  /**
   * @brief Unloads the plugin using the target platform's API if supported.
   */
  void unloadPlugin();

  ////////////
  // Variables
  std::filesystem::path m_Path;
  void* m_Handle = nullptr;
  std::shared_ptr<AbstractPlugin> m_Plugin;
};
} // namespace complex
