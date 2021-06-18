#pragma once

#include <memory>
#include <string>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT PluginLoader
{
public:
  /**
   * @brief Constructs a PluginLoader targetting the specified path.
   * The plugin is loaded upon construction and unloaded when the object is
   * destroyed.
   * @param path
   */
  PluginLoader(const std::string& path = "");

  virtual ~PluginLoader();

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
  std::string m_Path;
  void* m_Handle = nullptr;
  std::shared_ptr<AbstractPlugin> m_Plugin;
};

typedef AbstractPlugin* (*InitPluginFunc)();
} // namespace complex
