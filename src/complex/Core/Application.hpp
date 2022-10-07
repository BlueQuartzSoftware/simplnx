#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/Filter/FilterList.hpp"
//#include "complex/Utilities/Parsing/HDF5/H5DataFactoryManager.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractPlugin;
class DataIOCollection;
class IDataIOManager;
class JsonPipelineBuilder;

#if 0
namespace Zarr
{
class DataFactoryManager;
}
#endif

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
  using name_type_map = std::map<std::string, DataObject::Type>;

  /**
   * @brief Constructs an Application using default values and replaces the
   * current Instance pointer.
   */
  Application();

  /**
   * @brief Constructs an Application accepting a set of command line arguments.
   *
   * The current Application instance is replaced with the constructed Application.
   * @param argc
   * @param argv
   */
  Application(int argc, char** argv);

  /**
   * @brief Destroys the Application. If the destroyed Application matches the
   * current Application::Instance(), the instance pointer is set to nullptr.
   */
  ~Application();

  /**
   * @brief Returns a pointer to the current Application. This pointer is
   * replaced when a new complex::Application is created, replacing the
   * previous value. If the current Application is destroyed, this method will
   * return nullptr until a new Application is created.
   * @return Application*
   */
  static Application* Instance();

  static Application* GetOrCreateInstance();

  /**
   * @brief Finds and loads plugins in the target directory.
   *
   * Plugins are found by using the file extension of ".complex".
   * @param pluginDir
   */
  void loadPlugins(const std::filesystem::path& pluginDir, bool verbose = false);

  /**
   * @brief Returns a pointer to the Application's FilterList.
   *
   * This pointer is owned by the Application and will remain valid for as long
   * as the Application exists.
   * @return FilterList*
   */
  FilterList* getFilterList() const;

  /**
   * @brief Convenience method to return the loaded plugins
   * @return
   */
  std::unordered_set<AbstractPlugin*> getPluginList() const;

  /**
   * @brief Returns the loaded plugin with the given uuid.
   * Returns nullptr if no match.
   * @param pluginName
   * @return
   */
  const AbstractPlugin* getPlugin(const Uuid& uuid) const;

  /**
   * @brief Returns a pointer to the JsonPipelineBuilder. It is the caller's
   * responsibility to delete the pointer when they are done with it.
   * @return JsonPipelineBuilder*
   */
  JsonPipelineBuilder* getPipelineBuilder() const;

#if 0
  /**
   * @brief Returns a pointer to the Application's H5::DataFactoryManager.
   *
   * The pointer is owned by the Application and will remain valid for as long
   * as the Application exists.
   * @return DataFactoryManager*
   */
  H5::DataFactoryManager* getH5FactoryManager() const;
#endif

  std::shared_ptr<DataIOCollection> getIOCollection() const;

  std::shared_ptr<IDataIOManager> getIOManager(const std::string& formatName) const;

  template <typename T>
  std::shared_ptr<T> getIOManagerAs(const std::string& formatName) const
  {
    return std::dynamic_pointer_cast<T>(getIOManager(formatName));
  }

#if 0
  Zarr::DataFactoryManager* getZarrFactoryManager() const;
#endif

  /**
   * @brief Returns a filepath pointing to the current executable.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentPath() const;

  /**
   * @brief Returns a filepath pointing to the current executable's parent directory.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentDir() const;

  void addDataType(DataObject::Type type, const std::string& name);

  DataObject::Type getDataType(const std::string& name) const;

private:
  /**
   * @brief Assigns Application as the current instance and sets the current
   * executable path.
   */
  void assignInstance();

  void initDefaultDataTypes();

  /**
   * @brief Loads the plugin at the specified filepath and updates the
   * FilterList with the new IFilters.
   * @param path
   */
  void loadPlugin(const std::filesystem::path& path, bool verbose = false);

  //////////////////
  // Static Variable
  static Application* s_Instance;

  ////////////
  // Variables
  std::unique_ptr<complex::FilterList> m_FilterList;
  std::filesystem::path m_CurrentPath = "";
  // std::unique_ptr<H5::DataFactoryManager> m_DataReader;
  std::shared_ptr<DataIOCollection> m_DataIOCollection;
  name_type_map m_NamedTypesMap;
  // std::unique_ptr<Zarr::DataFactoryManager> m_ZarrFactoryManager;
};
} // namespace complex
