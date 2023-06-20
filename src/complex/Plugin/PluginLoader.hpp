#pragma once

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <memory>

namespace complex
{
class COMPLEX_EXPORT IPluginLoader
{
public:
  virtual ~IPluginLoader() noexcept = default;

  virtual bool isLoaded() const = 0;

  virtual AbstractPlugin* getPlugin() = 0;

  virtual const AbstractPlugin* getPlugin() const = 0;

protected:
  IPluginLoader() = default;
};

class COMPLEX_EXPORT InMemoryPluginLoader : public IPluginLoader
{
public:
  InMemoryPluginLoader(std::shared_ptr<AbstractPlugin> plugin)
  : IPluginLoader()
  , m_Plugin(std::move(plugin))
  {
  }

  ~InMemoryPluginLoader() noexcept override = default;

  bool isLoaded() const override
  {
    return m_Plugin != nullptr;
  }

  AbstractPlugin* getPlugin() override
  {
    return m_Plugin.get();
  }

  const AbstractPlugin* getPlugin() const override
  {
    return m_Plugin.get();
  }

private:
  std::shared_ptr<AbstractPlugin> m_Plugin;
};

/**
 * @class PluginLoader
 * @brief The PluginLoader class is the control wrapper around loading,
 * unloading, and accessing plugins. The implementation of PluginLoader is
 * provided for each operating system, but the public API remains the same.
 */
class COMPLEX_EXPORT PluginLoader : public IPluginLoader
{
public:
  /**
   * @brief Constructs a PluginLoader targeting the specified path.
   * The plugin is loaded upon construction and unloaded when the object is
   * destroyed.
   * @param path
   */
  PluginLoader(const std::filesystem::path& path);

  ~PluginLoader() noexcept override;

  /**
   * @brief Returns true if the plugin is loaded. Returns false otherwise.
   * @return bool
   */
  bool isLoaded() const override;

  /**
   * @brief Returns a pointer to the loaded plugin.
   * @return AbstractPlugin*
   */
  AbstractPlugin* getPlugin() override;

  /**
   * @brief Returns a pointer to the loaded plugin.
   * @return AbstractPlugin*
   */
  const AbstractPlugin* getPlugin() const override;

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
