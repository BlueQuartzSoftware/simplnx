#include "Application.hpp"

#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <fmt/core.h>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"

using namespace nx::core;

namespace
{
Result<std::filesystem::path> findCurrentPath()
{
#if defined(__linux__)
  std::vector<char> buffer(PATH_MAX + 1);
  ssize_t bytesWritten = readlink("/proc/self/exe", buffer.data(), buffer.size());
  if(bytesWritten < 0)
  {
    return MakeErrorResult<std::filesystem::path>(-10, fmt::format("Failed to get executable path: {}", strerror(errno)));
  }
  if(bytesWritten >= static_cast<ssize_t>(buffer.size()))
  {
    return MakeErrorResult<std::filesystem::path>(-11, "Failed to get executable path. Path too long for buffer.");
  }
  buffer[bytesWritten] = '\0';
  return {std::filesystem::path(buffer.data())};
#elif defined(_WIN32)
  std::vector<WCHAR> buffer(MAX_PATH + 1);
  DWORD bytesWritten = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if(bytesWritten == 0)
  {
    return MakeErrorResult<std::filesystem::path>(-12, fmt::format("Failed to get executable path: {}", GetLastError()));
  }
  if(bytesWritten >= static_cast<DWORD>(buffer.size()))
  {
    return MakeErrorResult<std::filesystem::path>(-13, "Failed to get executable path. Path too long for buffer.");
  }
  return {std::filesystem::path(buffer.data())};
#elif defined(__APPLE__)
  std::vector<char> buffer(1024 + 1);
  uint32 size = static_cast<uint32>(buffer.size());
  int result = _NSGetExecutablePath(buffer.data(), &size);
  if(result != 0)
  {
    return MakeErrorResult<std::filesystem::path>(-14, "Failed to get executable path. Path too long for buffer.");
  }
  return {std::filesystem::path(buffer.data())};
#else
  static_assert(false, "Unsupported platform for findCurrentPath()");
#endif
}

std::string getApplicationName(Application* app)
{
  return "DREAM3DNX";
}
} // namespace

std::shared_ptr<Application> Application::s_Instance = nullptr;

Application::Application()
: m_FilterList(std::make_unique<FilterList>())
, m_DataIOCollection(std::make_shared<DataIOCollection>())
{
  auto result = initialize();
  if(result.invalid())
  {
    // Can't propagate error from constructor, so log it
    fmt::print(stderr, "Error initializing application: {}\n", result.errors()[0].message);
  }
}

Application::Application(int argc, char** argv)
: Application()
{
  // Initialization already happened in delegated constructor
}

Result<> Application::initialize()
{
  Result<> combinedResult;

  auto prefsResult = loadPreferences();
  if(prefsResult.invalid())
  {
    // Non-fatal: can continue with default preferences
    combinedResult = MergeResults(std::move(combinedResult), std::move(prefsResult));
  }

  auto pathResult = findCurrentPath();
  if(pathResult.invalid())
  {
    return MakeErrorResult(-5, fmt::format("Failed to determine executable path: {}", pathResult.errors()[0].message));
  }
  m_CurrentPath = pathResult.value();

  initDefaultDataTypes();
  return combinedResult;
}

void Application::initDefaultDataTypes()
{
  addDataType(DataObject::Type::DynamicListArray, "DynamicListArray");
  addDataType(DataObject::Type::ScalarData, "ScalarData");
  addDataType(DataObject::Type::DataGroup, "DataGroup");
  addDataType(DataObject::Type::AttributeMatrix, "AttributeMatrix");
  addDataType(DataObject::Type::DataArray, "Data Array<T>");
  addDataType(DataObject::Type::RectGridGeom, "Rect Grid Geom");
  addDataType(DataObject::Type::ImageGeom, "Image Geom");
  addDataType(DataObject::Type::VertexGeom, "Vertex Geom");
  addDataType(DataObject::Type::EdgeGeom, "Edge Geom");
  addDataType(DataObject::Type::QuadGeom, "Quad Geom");
  addDataType(DataObject::Type::TriangleGeom, "Triangle Geom");
  addDataType(DataObject::Type::HexahedralGeom, "Hexahedral Geom");
  addDataType(DataObject::Type::TetrahedralGeom, "Tetrahedral Geom");
  addDataType(DataObject::Type::NeighborList, "NeighborList");
  addDataType(DataObject::Type::StringArray, "String Array");
}

Application::~Application()
{
  auto result = savePreferences();
  if(result.invalid())
  {
    // Can't propagate error from destructor, so log it
    fmt::print(stderr, "Error saving preferences in destructor: {}\n", result.errors().empty() ? "unknown error" : result.errors()[0].message);
  }
  s_Instance = nullptr;
}

std::shared_ptr<Application> Application::Instance()
{
  return s_Instance;
}

std::shared_ptr<Application> Application::GetOrCreateInstance()
{
  if(s_Instance == nullptr)
  {
    s_Instance = std::shared_ptr<Application>(new Application());
  }
  return s_Instance;
}

void Application::DeleteInstance()
{
  if(s_Instance != nullptr)
  {
    auto result = s_Instance->savePreferences();
    if(result.invalid())
    {
      // Can't propagate error from static function, so log it
      fmt::print(stderr, "Error saving preferences on shutdown: {}\n", result.errors().empty() ? "unknown error" : result.errors()[0].message);
    }
  }
  s_Instance = nullptr;
}

std::filesystem::path Application::getCurrentPath() const
{
  return m_CurrentPath;
}

std::filesystem::path Application::getCurrentDir() const
{
  return m_CurrentPath.parent_path();
}

Result<> Application::loadPreferences()
{
  if(m_Preferences == nullptr)
  {
    m_Preferences = std::make_unique<Preferences>();
  }
  std::string applicationName = getApplicationName(this);
  const auto filepath = Preferences::DefaultFilePath(applicationName);

  auto result = m_Preferences->loadFromFile(filepath);
  if(result.invalid())
  {
    return MakeErrorResult(-1, fmt::format("Could not load preferences from '{}': {}", filepath.string(), result.errors().empty() ? "unknown error" : result.errors()[0].message));
  }
  return result;
}
Result<> Application::savePreferences()
{
  if(m_Preferences == nullptr)
  {
    return MakeErrorResult(-2, "No preferences to save");
  }
  std::string applicationName = getApplicationName(this);
  const auto filepath = Preferences::DefaultFilePath(applicationName);

  auto result = m_Preferences->saveToFile(filepath);
  if(result.invalid())
  {
    return MakeErrorResult(-3, fmt::format("Failed to save preferences to '{}': {}", filepath.string(), result.errors().empty() ? "unknown error" : result.errors()[0].message));
  }
  return result;
}

std::optional<Uuid> Application::getSimplnxUuid(const Uuid& simplUuid)
{
  for(usize index = 0; index < m_Simpl_Uuids.size(); index++)
  {
    if(m_Simpl_Uuids[index] == simplUuid)
    {
      return m_Simplnx_Uuids[index];
    }
  }

  return {};
}

std::vector<Uuid> Application::getSimplUuid(const Uuid& simplnxUuid)
{
  std::vector<usize> indices;
  for(usize index = 0; index < m_Simplnx_Uuids.size(); index++)
  {
    if(m_Simplnx_Uuids[index] == simplnxUuid)
    {
      indices.push_back(index);
    }
  }
  std::vector<Uuid> uuidList;
  for(const auto& index : indices)
  {
    uuidList.push_back(m_Simpl_Uuids[index]);
  }
  return uuidList;
}

Result<> Application::loadPlugins(const std::filesystem::path& pluginDir, bool verbose)
{
  if(!std::filesystem::exists(pluginDir))
  {
    if(verbose)
    {
      fmt::print("Plugin Directory {} does not exist. Skipping\n", pluginDir.string());
    }
    return MakeErrorResult(-20, fmt::format("Plugin directory '{}' does not exist", pluginDir.string()));
  }

  if(!std::filesystem::is_directory(pluginDir))
  {
    return MakeErrorResult(-21, fmt::format("Path '{}' is not a directory", pluginDir.string()));
  }

  if(verbose)
  {
    fmt::print("Loading Plugins from {}\n", pluginDir.string());
  }

  Result<> combinedResult;

  try
  {
    for(const auto& entry : std::filesystem::directory_iterator(pluginDir))
    {
      std::filesystem::path path = entry.path();
#ifdef NDEBUG // Release mode
      if(!StringUtilities::ends_with(path.string(), "_d.simplnx") && StringUtilities::ends_with(path.string(), ".simplnx"))
#else
      if(StringUtilities::ends_with(path.string(), "_d.simplnx"))
#endif
      {
        auto result = loadPlugin(path, verbose);
        if(result.invalid())
        {
          // Accumulate errors but continue loading other plugins
          combinedResult = MergeResults(std::move(combinedResult), std::move(result));
        }
      }
    }
  } catch(const std::filesystem::filesystem_error& ex)
  {
    return MakeErrorResult(-22, fmt::format("Filesystem error loading plugins from '{}': {}", pluginDir.string(), ex.what()));
  }

  return combinedResult;
}

FilterList* Application::getFilterList() const
{
  return m_FilterList.get();
}

std::unordered_set<AbstractPlugin*> Application::getPluginList() const
{
  return m_FilterList->getLoadedPlugins();
}

const AbstractPlugin* Application::getPlugin(const Uuid& uuid) const
{
  std::unordered_set<AbstractPlugin*> plugins = m_FilterList->getLoadedPlugins();
  for(const auto* plugin : plugins)
  {
    if(plugin->getId() == uuid)
    {
      return plugin;
    }
  }
  return nullptr;
}

Preferences* Application::getPreferences()
{
  return m_Preferences.get();
}

JsonPipelineBuilder* Application::getPipelineBuilder() const
{
  return nullptr;
}

std::shared_ptr<DataIOCollection> Application::getIOCollection() const
{
  return m_DataIOCollection;
}

std::shared_ptr<IDataIOManager> Application::getIOManager(const std::string& formatName) const
{
  return m_DataIOCollection->getManager(formatName);
}

Result<> Application::loadPlugin(const std::filesystem::path& path, bool verbose)
{
  if(verbose)
  {
    fmt::print("Loading Plugin: {}\n", path.string());
  }

  auto pluginLoader = std::make_shared<PluginLoader>(path);
  auto addResult = getFilterList()->addPlugin(pluginLoader);
  if(addResult.invalid())
  {
    return MakeErrorResult(-30, fmt::format("Failed to add plugin from '{}': {}", path.string(), addResult.errors().empty() ? "unknown error" : addResult.errors()[0].message));
  }

  auto plugin = pluginLoader->getPlugin();
  if(plugin == nullptr)
  {
    return MakeErrorResult(-31, fmt::format("Plugin loaded from '{}' but returned null", path.string()));
  }

  // Check for duplicate UUIDs
  AbstractPlugin::SIMPLMapType simplToSimplnxUuids = plugin->getSimplToSimplnxMap();
  for(auto const& [simplUuid, simplData] : simplToSimplnxUuids)
  {
    for(const auto& uuid : m_Simpl_Uuids)
    {
      if(uuid == simplUuid)
      {
        return MakeErrorResult(-32, fmt::format("Duplicate UUIDs found in SIMPL UUID maps! UUID: {} Plugin: {}", simplUuid.str(), plugin->getName()));
      }
    }
    m_Simpl_Uuids.push_back(simplUuid);
    m_Simplnx_Uuids.push_back(simplData.simplnxUuid);
  }

  if(m_Simpl_Uuids.size() != m_Simplnx_Uuids.size())
  {
    return MakeErrorResult(-33, fmt::format("UUID maps are not of the same size! SIMPL: {} Simplnx: {}", m_Simpl_Uuids.size(), m_Simplnx_Uuids.size()));
  }

  for(const auto& pluginIO : plugin->getDataIOManagers())
  {
    m_DataIOCollection->addIOManager(pluginIO);
  }

  return {};
}

void Application::addDataType(DataObject::Type type, const std::string& name)
{
  m_NamedTypesMap[name] = type;
}

DataObject::Type Application::getDataType(const std::string& name) const
{
  if(m_NamedTypesMap.find(name) == m_NamedTypesMap.end())
  {
    return DataObject::Type::DataObject;
  }
  return m_NamedTypesMap.at(name);
}

std::vector<std::string> Application::getDataStoreFormats() const
{
  return m_DataIOCollection->getFormatNames();
}
