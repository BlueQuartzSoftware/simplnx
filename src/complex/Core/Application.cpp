#include "Application.hpp"

#include "complex/Filter/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Plugin/PluginLoader.hpp"
#include "complex/Utilities/StringUtilities.hpp"

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

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "complex/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "complex/Filter/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

namespace
{
std::filesystem::path findCurrentPath()
{
#if defined(__linux__)
  std::vector<char> buffer(PATH_MAX + 1);
  ssize_t bytesWritten = readlink("/proc/self/exe", buffer.data(), buffer.size());
  if(bytesWritten < 0)
  {
    throw std::runtime_error("Failed to get executable path");
  }
  if(bytesWritten >= buffer.size())
  {
    throw std::runtime_error("Failed to get executable path. Path too long for buffer.");
  }
  buffer[bytesWritten] = '\0';
  return std::filesystem::path(buffer.data());
#elif defined(_WIN32)
  std::vector<WCHAR> buffer(MAX_PATH + 1);
  DWORD bytesWritten = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if(bytesWritten == 0)
  {
    throw std::runtime_error("Failed to get executable path");
  }
  if(bytesWritten >= static_cast<DWORD>(buffer.size()))
  {
    throw std::runtime_error("Failed to get executable path. Path too long for buffer.");
  }
  return std::filesystem::path(buffer.data());
#elif defined(__APPLE__)
  std::vector<char> buffer(1024 + 1);
  uint32 size = static_cast<uint32>(buffer.size());
  int result = _NSGetExecutablePath(buffer.data(), &size);
  if(result != 0)
  {
    throw std::runtime_error("Failed to get executable path. Path too long buffer.");
  }
  return std::filesystem::path(buffer.data());
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
  initialize();
}

Application::Application(int argc, char** argv)
: Application()
{
  initialize();
}

void Application::initialize()
{
  loadPreferences();
  m_CurrentPath = findCurrentPath();
  initDefaultDataTypes();
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
  savePreferences();
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
    s_Instance->savePreferences();
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

void Application::loadPreferences()
{
  if(m_Preferences == nullptr)
  {
    m_Preferences = std::make_unique<Preferences>();
  }
  std::string applicationName = getApplicationName(this);
  const auto filepath = Preferences::DefaultFilePath(applicationName);
  m_Preferences->loadFromFile(filepath);
}
void Application::savePreferences()
{
  if(m_Preferences == nullptr)
  {
    return;
  }
  std::string applicationName = getApplicationName(this);
  const auto filepath = Preferences::DefaultFilePath(applicationName);
  m_Preferences->saveToFile(filepath);
}

std::optional<Uuid> Application::getComplexUuid(const Uuid& simplUuid)
{
  for(usize index = 0; index < m_Simpl_Uuids.size(); index++)
  {
    if(m_Simpl_Uuids[index] == simplUuid)
    {
      return m_Complex_Uuids[index];
    }
  }

  return {};
}

std::vector<Uuid> Application::getSimplUuid(const Uuid& complexUuid)
{
  std::vector<usize> indices;
  for(usize index = 0; index < m_Complex_Uuids.size(); index++)
  {
    if(m_Complex_Uuids[index] == complexUuid)
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

void Application::loadPlugins(const std::filesystem::path& pluginDir, bool verbose)
{
  if(verbose)
  {
    fmt::print("Loading Plugins from {}\n", pluginDir.string());
  }
  for(const auto& entry : std::filesystem::directory_iterator(pluginDir))
  {
    std::filesystem::path path = entry.path();
#ifdef NDEBUG // Release mode
    if(!StringUtilities::ends_with(path.string(), "_d.complex") && StringUtilities::ends_with(path.string(), ".complex"))
#else
    if(StringUtilities::ends_with(path.string(), "_d.complex"))
#endif
    {
      loadPlugin(path, verbose);
    }
  }
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

void Application::loadPlugin(const std::filesystem::path& path, bool verbose)
{
  if(verbose)
  {
    fmt::print("Loading Plugin: {}\n", path.string());
  }
  auto pluginLoader = std::make_shared<PluginLoader>(path);
  if(getFilterList()->addPlugin(pluginLoader).invalid())
  {
    return;
  }

  auto plugin = pluginLoader->getPlugin();
  if(plugin == nullptr)
  {
    return;
  }

  std::map<Uuid, Uuid> simplToComplexUuids = plugin->getSimplToComplexMap();
  for(auto const& [simplUuid, complexUuid] : simplToComplexUuids)
  {
    for(const auto& uuid : m_Simpl_Uuids)
    {
      if(uuid == simplUuid)
      {
        throw std::runtime_error(fmt::format("Duplicate UUIDs found in the SIMPL UUID maps! UUID: {} Plugin: {}", simplUuid.str(), plugin->getName()));
      }
    }
    m_Simpl_Uuids.push_back(simplUuid);
    m_Complex_Uuids.push_back(complexUuid);
  }

  if(m_Simpl_Uuids.size() != m_Complex_Uuids.size())
  {
    throw std::runtime_error(fmt::format("UUID maps are not of the same size! SIMPL UUID Vector size: {} Complex UUID Vector size: {}", m_Simpl_Uuids.size(), m_Complex_Uuids.size()));
  }

  for(const auto& pluginIO : plugin->getDataIOManagers())
  {
    m_DataIOCollection->addIOManager(pluginIO);
  }
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
