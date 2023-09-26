#pragma once

#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace complex
{
namespace H5
{
class IDataFactory;
}

class IDataIOManager;

/**
 * @class AbstractPlugin
 * @brief The AbstractPlugin class is the base class for all C++ plugins for
 * use in the application. The AbstractPlugin class provides an interface for
 * retrieving information about the plugin and creating filters within the
 * plugin.
 */
class COMPLEX_EXPORT AbstractPlugin
{
public:
  using IdType = Uuid;
  using FilterContainerType = std::unordered_set<FilterHandle>;
  using IOManagerPointer = std::shared_ptr<IDataIOManager>;
  using IOManagersContainerType = std::vector<IOManagerPointer>;

  struct COMPLEX_EXPORT SIMPLData
  {
    using ConversionFunction = std::function<Result<Arguments>(const nlohmann::json&)>;

    Uuid complexUuid;
    ConversionFunction convertJson;
  };

  using SIMPLMapType = std::map<Uuid, SIMPLData>;

  virtual ~AbstractPlugin();

  /**
   * @brief Returns the plugin's name.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns the plugin's description.
   * @return std::string
   */
  std::string getDescription() const;

  /**
   * @brief Returns the plugin's ID.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Checks if the plugin contains a filter with the given ID. Returns
   * true if the plugin contains the filter. Returns false otherwise.
   * @param identifier
   * @return bool
   */
  bool containsFilterId(FilterHandle::FilterIdType identifier) const;

  /**
   * @brief Create's an IFilter with the specified ID. If the plugin
   * does not contain a filter with the specified ID, this function returns
   * nullptr. Otherwise, this returns a std::unique_ptr to the created IFilter.
   * @param filterId
   * @return IFilter::UniquePointer
   */
  IFilter::UniquePointer createFilter(FilterHandle::FilterIdType filterId) const;

  /**
   * @brief Returns a set of FilterHandles pointing to each of the filters
   * contained in the plugin.
   * @return std::unordered_set<complex::FilterHandle>
   */
  FilterContainerType getFilterHandles() const;

  /**
   * @brief
   * @return
   */
  FilterContainerType::size_type getFilterCount() const;

  /**
   * @brief Returns the plugin's vendor name.
   * @return std::string
   */
  std::string getVendor() const;

  /**
   * @brief Returns a collection of DataStructure IO managers available
   * through the plugin for use in reading or writing the DataStructure
   * to a specific format.
   * @return IOManagersContainerType
   */
  IOManagersContainerType getDataIOManagers() const;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return SIMPLMapType
   */
  virtual SIMPLMapType getSimplToComplexMap() const = 0;

protected:
  /**
   * @brief Constructs a new AbstractPlugin. Takes an ID, name, description,
   * and vendor.
   * @param identifier
   * @param name
   * @param description
   */
  AbstractPlugin(IdType identifier, const std::string& name, const std::string& description, const std::string& vendor);

  /**
   * @brief Records information for creating a filter using the provided
   * FilterCreationFunc. The filter ID is provided by the filter created by the
   * FilterCreationFunc. If the FilterCreationFunc fails to create a filter,
   * this method does nothing.
   * @param filterFunc
   */
  void addFilter(FilterCreationFunc filterFunc);

  /**
   * @brief Inserts an IOManager into the plugin for use throughout complex once the plugin has been loaded.
   * @param ioManager
   */
  void addDataIOManager(const IOManagerPointer& ioManager);

  /**
   * @brief Adds a default value to the Preferences for the current plugin.
   * @param keyName
   * @param value
   */
  void addDefaultValue(std::string keyName, const nlohmann::json& value);

private:
  IdType m_Id;
  std::string m_Name;
  std::string m_Description;
  std::string m_Vendor;
  std::unordered_set<FilterHandle> m_FilterHandles;
  std::unordered_map<FilterHandle::FilterIdType, FilterCreationFunc> m_InitializerMap;
  IOManagersContainerType m_IOManagers;
};

using CreatePluginFunc = AbstractPlugin* (*)();
using DestroyPluginFunc = bool (*)(AbstractPlugin*);
} // namespace complex

#define COMPLEX_CREATE_PLUGIN_FUNC COMPLEX_CreatePlugin
#define COMPLEX_DESTROY_PLUGIN_FUNC COMPLEX_DestroyPlugin

#define COMPLEX_STRINGIFY_IMPL(x) #x
#define COMPLEX_STRINGIFY(x) COMPLEX_STRINGIFY_IMPL(x)

#define COMPLEX_CREATE_PLUGIN_FUNC_NAME COMPLEX_STRINGIFY(COMPLEX_CREATE_PLUGIN_FUNC)
#define COMPLEX_DESTROY_PLUGIN_FUNC_NAME COMPLEX_STRINGIFY(COMPLEX_DESTROY_PLUGIN_FUNC)

#if defined(_WIN32)
#define COMPLEX_PLUGIN_EXPORT __declspec(dllexport)
#else
#define COMPLEX_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#define COMPLEX_DEF_PLUGIN_IMPL(pluginType, createName, destroyName)                                                                                                                                   \
  extern "C" {                                                                                                                                                                                         \
  COMPLEX_PLUGIN_EXPORT complex::AbstractPlugin* createName()                                                                                                                                          \
  {                                                                                                                                                                                                    \
    return new pluginType();                                                                                                                                                                           \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  COMPLEX_PLUGIN_EXPORT bool destroyName(complex::AbstractPlugin* plugin)                                                                                                                              \
  {                                                                                                                                                                                                    \
    auto convertedPlugin = dynamic_cast<pluginType*>(plugin);                                                                                                                                          \
    if(convertedPlugin == nullptr)                                                                                                                                                                     \
    {                                                                                                                                                                                                  \
      return false;                                                                                                                                                                                    \
    }                                                                                                                                                                                                  \
    delete plugin;                                                                                                                                                                                     \
    return true;                                                                                                                                                                                       \
  }                                                                                                                                                                                                    \
  }

#define COMPLEX_DEF_PLUGIN(pluginType) COMPLEX_DEF_PLUGIN_IMPL(pluginType, COMPLEX_CREATE_PLUGIN_FUNC, COMPLEX_DESTROY_PLUGIN_FUNC)
