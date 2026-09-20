#pragma once

#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nx::core
{

namespace H5
{
class IDataFactory;
}

class IDataIOManager;

/**
 * @class AbstractPlugin
 * @brief Defines runtime metadata, filters, and I/O services for a plugin.
 *
 * Application owns loaded plugins. A plugin owns registered filter factories
 * and shared I/O-manager pointers.
 */
class SIMPLNX_EXPORT AbstractPlugin
{
public:
  using IdType = Uuid;

  using FilterContainerType = std::unordered_set<FilterHandle>;

  using IOManagerPointer = std::shared_ptr<IDataIOManager>;

  using IOManagersContainerType = std::vector<IOManagerPointer>;

  /**
   * @struct SIMPLData
   * @brief Maps a legacy SIMPL filter to a simplnx filter.
   */
  struct SIMPLNX_EXPORT SIMPLData
  {
    using ConversionFunction = std::function<Result<Arguments>(const nlohmann::json&)>;

    Uuid simplnxUuid;               // Simplnx filter UUID that receives the converted arguments.
    ConversionFunction convertJson; // Converts legacy JSON to simplnx Arguments.
  };

  using SIMPLMapType = std::map<Uuid, SIMPLData>;

  virtual ~AbstractPlugin();

  std::string getName() const;

  std::string getDescription() const;

  IdType getId() const;

  bool containsFilterId(FilterHandle::FilterIdType identifier) const;

  /**
   * @brief Creates a filter from its registered factory.
   * @param filterId Filter UUID to create.
   * @return Owning filter pointer, or nullptr when no factory matches or returns null.
   */
  IFilter::UniquePointer createFilter(FilterHandle::FilterIdType filterId) const;

  FilterContainerType getFilterHandles() const;

  FilterContainerType::size_type getFilterCount() const;

  std::string getVendor() const;

  IOManagersContainerType getDataIOManagers() const;

  virtual SIMPLMapType getSimplToSimplnxMap() const = 0;

protected:
  AbstractPlugin(IdType identifier, const std::string& name, const std::string& description, const std::string& vendor);

  /**
   * @brief Registers a filter factory.
   * @param filterFunc Factory that returns a new filter.
   * @throws std::runtime_error if filterFunc returns null or duplicates a UUID.
   *
   * The method creates one filter to obtain and validate its UUID.
   */
  void addFilter(FilterCreationFunc filterFunc);

  void addDataIOManager(const IOManagerPointer& ioManager);

  /**
   * @brief Registers a plugin default preference value.
   * @param keyName Plugin preference key.
   * @param value Default JSON value.
   *
   * Application owns the shared Preferences instance.
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

/**
 * @brief Defines a C ABI plugin factory.
 *
 * The caller owns the returned plugin and releases it through DestroyPluginFunc.
 */
using CreatePluginFunc = AbstractPlugin* (*)();

/**
 * @brief Defines a C ABI plugin destruction function.
 *
 * The function deletes a plugin only when its concrete type matches the export.
 */
using DestroyPluginFunc = bool (*)(AbstractPlugin*);
} // namespace nx::core

#define SIMPLNX_CREATE_PLUGIN_FUNC SIMPLNX_CreatePlugin

#define SIMPLNX_DESTROY_PLUGIN_FUNC SIMPLNX_DestroyPlugin

#define SIMPLNX_STRINGIFY_IMPL(x) #x

#define SIMPLNX_STRINGIFY(x) SIMPLNX_STRINGIFY_IMPL(x)

#define SIMPLNX_CREATE_PLUGIN_FUNC_NAME SIMPLNX_STRINGIFY(SIMPLNX_CREATE_PLUGIN_FUNC)

#define SIMPLNX_DESTROY_PLUGIN_FUNC_NAME SIMPLNX_STRINGIFY(SIMPLNX_DESTROY_PLUGIN_FUNC)

#if defined(_WIN32)
#define SIMPLNX_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SIMPLNX_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/**
 * @def SIMPLNX_DEF_PLUGIN_IMPL
 * @brief Defines exported plugin creation and destruction functions.
 * @param pluginType Concrete AbstractPlugin type.
 * @param createName Exported factory symbol.
 * @param destroyName Exported destruction symbol.
 */
#define SIMPLNX_DEF_PLUGIN_IMPL(pluginType, createName, destroyName)                                                                                                                                   \
  extern "C" {                                                                                                                                                                                         \
  SIMPLNX_PLUGIN_EXPORT nx::core::AbstractPlugin* createName()                                                                                                                                         \
  {                                                                                                                                                                                                    \
    return new pluginType();                                                                                                                                                                           \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  SIMPLNX_PLUGIN_EXPORT bool destroyName(nx::core::AbstractPlugin* plugin)                                                                                                                             \
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

/**
 * @def SIMPLNX_DEF_PLUGIN
 * @brief Defines default exported plugin creation and destruction functions.
 * @param pluginType Concrete AbstractPlugin type.
 */
#define SIMPLNX_DEF_PLUGIN(pluginType) SIMPLNX_DEF_PLUGIN_IMPL(pluginType, SIMPLNX_CREATE_PLUGIN_FUNC, SIMPLNX_DESTROY_PLUGIN_FUNC)
