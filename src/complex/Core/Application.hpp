#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "complex/Core/FilterList.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractPlugin;
class JsonPipelineBuilder;
// class RestServer;

/**
 * @class Application
 * @brief The Application class serves as the core of the framework. The
 * Application instance provides access to the FilterList, PipelineBuilder,
 * and REST server. The Application handles loading available plugins into the
 * FilterList so that they can be used by the rest of the codebase.
 * When the Application is deleted, plugins are released and memory is cleaned
 * up. Pipelines or DataStructures are not cleaned up unless they are owned by
 * the REST server, but plugin-specific information or calculations will be
 * made unavailable.
 */
class COMPLEX_EXPORT Application
{
public:
  /**
   * @brief Default constructor
   */
  Application();

  /**
   * @brief Constructs an Application accepting a set of command line arguments.
   * @param argc
   * @param argv
   */
  Application(int argc, char** argv);

  virtual ~Application();

  /**
   * @brief Returns a pointer to the current Application.
   * @return Application*
   */
  static Application* Instance();

  /**
   * @brief Loads plugins in the target directory
   * @param pluginDir
   */
  void loadPlugins(const std::filesystem::path& pluginDir);

  /**
   * @brief Returns a pointer to the Application's FilterList.
   * @return FilterList*
   */
  FilterList* getFilterList() const;

  /**
   * @brief Starts a REST server using the target port.
   * @param port
   * @return RestServer*
   */
  // RestServer* startRestServer(int32_t port);

  /**
   * @brief Returns a pointer to the JsonPipelineBuilder. It is the caller's
   * responsibility to delete the pointer when they are done with it.
   * @return JsonPipelineBuilder*
   */
  JsonPipelineBuilder* getPipelineBuilder() const;

  /**
   * @brief Returns a filepath pointing to the executable.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentPath() const;

  /**
   * @brief Returns a filepath pointing to the executable's directory.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentDir() const;

protected:
private:
  /**
   * @brief Performs initialization work shared between constructors.
   */
  void initialize();

  /**
   * @brief Loads the plugin at the specified file path.
   * @param path
   */
  void loadPlugin(const std::string& path);

  //////////////////
  // Static Variable
  static Application* s_Instance;

  ////////////
  // Variables
  std::unique_ptr<complex::FilterList> m_FilterList;
  std::vector<std::unique_ptr<AbstractPlugin>> m_Plugins;
  // std::unique_ptr<complex::RestServer> m_RestServer;
  std::filesystem::path m_CurrentPath = "";
};
} // namespace complex
