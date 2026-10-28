#include "DataStructure.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataReparentedMessage.hpp"
#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/expected.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace
{
const std::string k_Delimiter = "|--";

/**
 * @brief Returns the lazy process-wide storage-format resolver.
 * @return Mutable reference to the default resolver pointer.
 *
 * In-memory storage keeps DataStructure usable before application startup sets
 * the resolver.
 */
std::shared_ptr<const nx::core::IDataStoreFormatResolver>& DefaultFormatResolverRef()
{
  static std::shared_ptr<const nx::core::IDataStoreFormatResolver> s_Default = std::make_shared<nx::core::InMemoryFormatResolver>();
  return s_Default;
}
} // namespace

namespace nx::core
{
/**
 * @struct ImportPublicationTestState
 * @brief Keeps action-local faults and failed-transaction owners in DataStructure.
 */
struct DataStructure::ImportPublicationTestState
{
  ImportPublicationTestState() = default;
  ~ImportPublicationTestState() noexcept = default;
  ImportPublicationTestState(const ImportPublicationTestState&) = delete;
  ImportPublicationTestState(ImportPublicationTestState&&) = delete;
  ImportPublicationTestState& operator=(const ImportPublicationTestState&) = delete;
  ImportPublicationTestState& operator=(ImportPublicationTestState&&) = delete;

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  ImportPublicationTestControl::Fault Fault = ImportPublicationTestControl::Fault::None;
  bool Consumed = false;
  bool RefuseCleanup = false;
  bool CleanupConsumed = false;
  std::shared_ptr<DataObject> RetainedOwner;
  std::vector<ImportPublicationRecord> RetainedRecords;
  DataStructure* Destination = nullptr;
  DataObject::IdType SavedNextId = 0;
  DataObject::IdType ReservedNextId = 0;
#endif
};

DataStructure::DataStructure()
: m_IsValid(true)
{
}

DataStructure::DataStructure(const DataStructure& dataStructure)
: m_DataObjects(dataStructure.m_DataObjects)
, m_RootGroup(dataStructure.m_RootGroup)
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
, m_FormatResolver(dataStructure.m_FormatResolver)
{
  // Keep copied objects alive while the root map rebinds their DataStructure.
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> sharedData;
  for(const auto& [identifier, dataWkPtr] : dataStructure.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[identifier] = copy;
      m_DataObjects[identifier] = copy;
    }
  }
  m_RootGroup.setDataStructure(this);
}

DataStructure::DataStructure(DataStructure&& dataStructure) noexcept
: m_DataObjects(std::move(dataStructure.m_DataObjects))
, m_RootGroup(std::move(dataStructure.m_RootGroup))
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
, m_FormatResolver(std::move(dataStructure.m_FormatResolver))
{
  m_RootGroup.setDataStructure(this);
}

DataStructure::~DataStructure()
{
  m_IsValid = false;
  for(auto& [identifier, weakDataPtr] : m_DataObjects)
  {
    if(auto sharedDataPtr = weakDataPtr.lock())
    {
      if(sharedDataPtr->getDataStructure() == this)
      {
        sharedDataPtr->setDataStructure(nullptr);
      }
    }
  }
}

void DataStructure::setFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  m_FormatResolver = std::move(resolver);
}

const IDataStoreFormatResolver& DataStructure::formatResolver() const
{
  if(m_FormatResolver != nullptr)
  {
    return *m_FormatResolver;
  }
  return *DefaultFormatResolverRef();
}

std::shared_ptr<const IDataStoreFormatResolver> DataStructure::getDefaultFormatResolver()
{
  return DefaultFormatResolverRef();
}

void DataStructure::setDefaultFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  if(resolver != nullptr)
  {
    DefaultFormatResolverRef() = std::move(resolver);
  }
}

DataObject::IdType DataStructure::generateId()
{
  return m_NextId++;
}

void DataStructure::setNextId(DataObject::IdType nextDataId)
{
  m_NextId = nextDataId;
}

size_t DataStructure::getSize() const
{
  return m_DataObjects.size();
}

void DataStructure::clear()
{
  auto topDataIds = m_RootGroup.getKeys();
  for(auto dataId : topDataIds)
  {
    removeData(dataId);
  }
  m_DataObjects.clear();
}

std::optional<DataObject::IdType> DataStructure::getId(const DataPath& path) const
{
  if(path.empty())
  {
    return {0};
  }
  const DataObject* dataObject = getData(path);
  if(nullptr == dataObject)
  {
    return std::nullopt;
  }
  return dataObject->getId();
}

LinkedPath DataStructure::getLinkedPath(const DataPath& path) const
{
  try
  {
    std::vector<DataObject::IdType> pathIds;
    const DataObject* data = m_RootGroup[path[0]];
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(usize i = 1; i < path.getLength(); i++)
    {
      std::string name = path[i];
      data = (*parent)[name];
      pathIds.push_back(data->getId());

      parent = dynamic_cast<const BaseGroup*>(data);
    }

    return LinkedPath(this, pathIds);
  } catch(const std::exception& e)
  {
    return LinkedPath();
  }
}

bool DataStructure::containsData(DataObject::IdType identifier) const
{
  return getData(identifier) != nullptr;
}

bool DataStructure::containsData(const DataPath& path) const
{
  return getData(path) != nullptr;
}

Result<LinkedPath> DataStructure::makePath(const DataPath& path)
{
  std::vector<DataObject::IdType> createdIds;

  try
  {
    std::vector<DataObject::IdType> pathIds;
    std::string name = path[0];
    const DataObject* data = m_RootGroup[name];
    if(data == nullptr)
    {
      data = nx::core::DataGroup::Create(*this, name);
      createdIds.push_back(data->getId());
    }
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(usize i = 1; i < path.getLength(); i++)
    {
      if(parent == nullptr)
      {
        return nx::core::MakeErrorResult<LinkedPath>(-3, fmt::format("Target parent object '{}' in path '{}' is not derived from BaseGroup.", name, path.toString()));
      }

      name = path[i];
      data = (*parent)[name];
      if(data == nullptr)
      {
        data = DataGroup::Create(*this, name, pathIds.back());
        createdIds.push_back(data->getId());
      }
      pathIds.push_back(data->getId());

      parent = dynamic_cast<const BaseGroup*>(data);
    }

    return {LinkedPath(this, pathIds)};
  } catch(const std::exception& e)
  {
    for(const auto& id : createdIds)
    {
      removeData(id);
    }

    return nx::core::MakeErrorResult<LinkedPath>(-2, fmt::format("Exception thrown when attempting to create a path '{}' in the DataStructure: '{}'", path.toString(), e.what()));
  }
}

std::vector<DataPath> DataStructure::getDataPathsForId(DataObject::IdType identifier) const
{
  auto* dataObject = getData(identifier);
  if(dataObject == nullptr)
  {
    return {};
  }
  return dataObject->getDataPaths();
}

std::vector<DataPath> DataStructure::getAllDataPaths() const
{
  std::vector<DataPath> dataPaths;
  for(const auto& [identifier, weakPtr] : m_DataObjects)
  {
    auto sharedPtr = weakPtr.lock();
    if(sharedPtr == nullptr)
    {
      continue;
    }

    auto localPaths = sharedPtr->getDataPaths();
    dataPaths.insert(dataPaths.end(), localPaths.begin(), localPaths.end());
  }
  return dataPaths;
}

std::vector<DataObject::IdType> DataStructure::getAllDataObjectIds() const
{
  std::vector<DataObject::IdType> dataIds;
  dataIds.reserve(m_DataObjects.size());
  for(const auto& [identifier, weakPtr] : m_DataObjects)
  {
    dataIds.push_back(identifier);
  }
  return dataIds;
}

DataObject* DataStructure::getData(DataObject::IdType identifier)
{
  auto iter = m_DataObjects.find(identifier);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& identifier)
{
  if(!identifier)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(identifier.value());
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* DataStructure::getData(const DataPath& path)
{
  if(path.empty())
  {
    return nullptr;
  }
  DataObject* targetObject = m_RootGroup[path[0]];
  for(usize index = 1; index < path.getLength(); index++)
  {
    if(targetObject == nullptr)
    {
      return nullptr;
    }
    if(!targetObject->isGroup())
    {
      return nullptr;
    }
    auto* groupObject = static_cast<BaseGroup*>(targetObject);
    DataObject* childObject = (*groupObject)[path[index]];
    if(childObject == nullptr)
    {
      return nullptr;
    }
    targetObject = childObject;
  }

  return targetObject;
}

DataObject& DataStructure::getDataRef(const DataPath& path)
{
  DataObject* object = getData(path);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Input Path '{}' does not exist", path.toString()));
  }
  return *object;
}

DataObject& DataStructure::getDataRef(DataObject::IdType identifier)
{
  DataObject* object = getData(identifier);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Id '{}' does not exist", identifier));
  }
  return *object;
}

DataObject* DataStructure::getData(const LinkedPath& path)
{
  return getData(path.getId());
}

const DataObject* DataStructure::getData(DataObject::IdType identifier) const
{
  auto iter = m_DataObjects.find(identifier);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

const DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& identifier) const
{
  if(!identifier)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(identifier.value());
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

const DataObject* DataStructure::getData(const DataPath& path) const
{
  if(path.empty())
  {
    return nullptr;
  }
  const DataObject* targetObject = m_RootGroup[path[0]];
  for(usize index = 1; index < path.getLength(); index++)
  {
    if(targetObject == nullptr)
    {
      return nullptr;
    }
    if(!targetObject->isGroup())
    {
      return nullptr;
    }
    const auto* groupObject = static_cast<const BaseGroup*>(targetObject);
    const DataObject* childObject = (*groupObject)[path[index]];
    if(childObject == nullptr)
    {
      return nullptr;
    }
    targetObject = childObject;
  }

  return targetObject;
}

const DataObject& DataStructure::getDataRef(const DataPath& path) const
{
  const DataObject* object = getData(path);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Input Path '{}' does not exist", path.toString()));
  }
  return *object;
}

const DataObject& DataStructure::getDataRef(DataObject::IdType identifier) const
{
  const DataObject* object = getData(identifier);
  if(object == nullptr)
  {
    throw std::out_of_range(fmt::format("DataStructure::getDataRef(): Id '{}' does not exist", identifier));
  }
  return *object;
}

const DataObject* DataStructure::getData(const LinkedPath& path) const
{
  return path.getData();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(DataObject::IdType id)
{
  if(m_DataObjects.find(id) == m_DataObjects.end())
  {
    return nullptr;
  }
  return m_DataObjects.at(id).lock();
}

std::shared_ptr<const DataObject> DataStructure::getSharedData(DataObject::IdType id) const
{
  if(m_DataObjects.find(id) == m_DataObjects.end())
  {
    return nullptr;
  }
  return m_DataObjects.at(id).lock();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(const DataPath& path)
{
  auto dataObject = getData(path);
  if(dataObject == nullptr)
  {
    return nullptr;
  }
  return m_DataObjects.at(dataObject->getId()).lock();
}

std::shared_ptr<const DataObject> DataStructure::getSharedData(const DataPath& path) const
{
  auto dataObject = getData(path);
  if(dataObject == nullptr)
  {
    return nullptr;
  }
  return m_DataObjects.at(dataObject->getId()).lock();
}

bool DataStructure::removeData(DataObject::IdType identifier)
{
  DataObject* data = getData(identifier);
  return removeData(data);
}

void DataStructure::setData(DataObject::IdType identifier, std::shared_ptr<DataObject> dataObject)
{
  if(dataObject == nullptr)
  {
    removeData(identifier);
    return;
  }

  m_DataObjects[identifier] = dataObject;
}

bool DataStructure::removeData(const std::optional<DataObject::IdType>& identifier)
{
  if(!identifier)
  {
    return false;
  }
  else
  {
    return removeData(identifier.value());
  }
}

bool DataStructure::removeData(const DataPath& path)
{
  DataObject* data = getData(path);
  return removeData(data);
}

bool DataStructure::removeData(DataObject* data)
{
  if(data == nullptr)
  {
    return false;
  }

  auto pathsToData = data->getDataPaths();
  auto parentIds = data->getParentIds();
  if(parentIds.size() == 0)
  {
    return removeTopLevel(data);
  }
  for(DataObject::IdType parentId : parentIds)
  {
    auto parent = getDataAs<BaseGroup>(parentId);
    if(!parent->remove(data))
    {
      return false;
    }
  }

  return true;
}

void DataStructure::dataDeleted(DataObject::IdType identifier, const std::string& name)
{
  if(!m_IsValid)
  {
    return;
  }

  auto msg = std::make_shared<DataRemovedMessage>(this, identifier, name);
  notify(msg);
}

std::vector<DataObject*> DataStructure::getTopLevelData() const
{
  std::vector<DataObject*> topLevel(m_RootGroup.getSize(), nullptr);
  usize index = 0;
  for(auto& iter : m_RootGroup)
  {
    auto obj = iter.second;
    topLevel[index++] = (obj.get());
  }
  return topLevel;
}

const DataMap& DataStructure::getDataMap() const
{
  return m_RootGroup;
}

DataMap& DataStructure::getRootGroup()
{
  return m_RootGroup;
}

bool DataStructure::insertTopLevel(const std::shared_ptr<DataObject>& obj)
{
  if(obj == nullptr)
  {
    return false;
  }

  if(m_RootGroup.contains(obj.get()) || m_RootGroup.contains(obj->getName()))
  {
    return false;
  }

  return m_RootGroup.insert(obj);
}

bool DataStructure::removeTopLevel(DataObject* data)
{
  std::string name = data->getName();
  if(!m_RootGroup.remove(data))
  {
    return false;
  }

  DataPath path({name});
  std::vector<DataPath> paths({path});
  return true;
}

bool DataStructure::finishAddingObject(const std::shared_ptr<DataObject>& dataObject, const std::optional<DataObject::IdType>& parent)
{
  if(parent.has_value() && containsData(*parent))
  {
    auto* parentContainer = dynamic_cast<BaseGroup*>(getData(*parent));
    if(parentContainer == nullptr)
    {
      return false;
    }
    if(!parentContainer->insert(dataObject))
    {
      return false;
    }
  }
  else if(!insertTopLevel(dataObject))
  {
    return false;
  }

  trackDataObject(dataObject);
  auto msg = std::make_shared<DataAddedMessage>(this, dataObject->getId());
  notify(msg);
  return true;
}

DataStructure::Iterator DataStructure::begin()
{
  return m_RootGroup.begin();
}

DataStructure::Iterator DataStructure::end()
{
  return m_RootGroup.end();
}

DataStructure::ConstIterator DataStructure::begin() const
{
  return m_RootGroup.begin();
}

DataStructure::ConstIterator DataStructure::end() const
{
  return m_RootGroup.end();
}

bool DataStructure::insert(const std::shared_ptr<DataObject>& dataObject, const DataPath& dataPath)
{
  if(dataObject == nullptr)
  {
    return false;
  }

  if(dataObject.get() == getData(dataObject->getId()))
  {
    return false;
  }

  if(getData(dataObject->getId()) != nullptr)
  {
    dataObject->setId(generateId());
  }

  // A new insertion starts with one requested placement and no inherited parents.
  dataObject->clearParents();

  if(dataPath.empty())
  {
    return insertIntoRoot(dataObject);
  }

  auto parentGroup = getDataAs<BaseGroup>(dataPath);
  return insertIntoParent(dataObject, parentGroup);
}

DataStructure::ImportPublicationCheckpoint::ImportPublicationCheckpoint(const DataStructure& destination) noexcept
: m_Destination(&destination)
, m_NextId(destination.m_NextId)
{
}

bool DataStructure::ImportPublicationCheckpoint::belongsTo(const DataStructure& destination) const noexcept
{
  return m_Destination == &destination;
}

DataObject::IdType DataStructure::ImportPublicationCheckpoint::nextId() const noexcept
{
  return m_NextId;
}

DataObject::IdType DataStructure::getLiveObjectCount() const
{
  DataObject::IdType count = 0;
  for(const auto& entry : m_DataObjects)
  {
    if(!entry.second.expired())
    {
      ++count;
    }
  }
  return count;
}

std::set<DataObject::IdType> DataStructure::getOccupiedObjectIds() const
{
  std::set<DataObject::IdType> occupiedIds;
  for(const auto& entry : m_DataObjects)
  {
    occupiedIds.insert(entry.first);
  }
  // Registry enumeration omits hierarchy-only placements. Inspect actual maps before copying the destination.
  std::vector<const DataMap*> pendingMaps{&getDataMap()};
  std::unordered_set<const DataObject*> visitedObjects;
  while(!pendingMaps.empty())
  {
    const auto* mapPtr = pendingMaps.back();
    pendingMaps.pop_back();
    for(const auto& [id, owner] : *mapPtr)
    {
      occupiedIds.insert(id);
      if(owner == nullptr)
      {
        continue;
      }
      occupiedIds.insert(owner->getId());
      if(visitedObjects.insert(owner.get()).second)
      {
        if(const auto* groupPtr = dynamic_cast<const BaseGroup*>(owner.get()); groupPtr != nullptr)
        {
          pendingMaps.push_back(&groupPtr->getDataMap());
        }
      }
    }
  }

  return occupiedIds;
}

Result<> DataStructure::prepareImportPublicationRecords(DataStructure& prepared, const std::vector<DataPath>& paths, const fs::path& sourceFile, std::vector<ImportPublicationRecord>& records,
                                                        DataObject::IdType& reservedNextId)
{
  constexpr auto maximumId = std::numeric_limits<DataObject::IdType>::max();
  records.reserve(paths.size());
  std::map<DataPath, std::shared_ptr<BaseGroup>> publicationGroups;
  std::set<DataPath> selectedPaths;
  std::unordered_map<const BaseGroup*, std::set<std::string>> selectedPlacements;
  std::optional<std::set<DataObject::IdType>> occupiedIds;
  for(const auto& path : paths)
  {
    const auto object = prepared.getSharedData(path);
    if(object == nullptr)
    {
      // Recovery may remove an invalid leaf with a warning. Preserve that partial-recovery contract.
      continue;
    }
    if(containsData(path))
    {
      return MakeErrorResult(-6203, fmt::format("Cannot publish '{}' from '{}': the destination path already exists. Rename it or exclude this path.", path.toString(), sourceFile.string()));
    }
    if(!selectedPaths.insert(path).second)
    {
      return MakeErrorResult(-6203, fmt::format("Cannot publish '{}' from '{}': the same path is selected more than once.", path.toString(), sourceFile.string()));
    }
    if(!occupiedIds.has_value())
    {
      occupiedIds = getOccupiedObjectIds();
    }
    ImportPublicationRecord record;
    record.Owner = makeImportPublicationCopy(*object);
    record.Id = object->getId();
    record.Path = path;
    if(record.Owner == nullptr || record.Id == 0 || record.Id == maximumId || !occupiedIds->insert(record.Id).second)
    {
      return MakeErrorResult(-6213, fmt::format("Cannot publish '{}' from '{}': identifier {} or its prepared owner is invalid.", path.toString(), sourceFile.string(), record.Id));
    }
    const auto parentPath = path.getParent();
    if(!parentPath.empty())
    {
      auto parent = publicationGroups.find(parentPath);
      record.Parent = parent == publicationGroups.end() ? std::dynamic_pointer_cast<BaseGroup>(getSharedData(parentPath)) : parent->second;
      if(record.Parent == nullptr)
      {
        return MakeErrorResult(-6202, fmt::format("Cannot publish '{}': destination parent '{}' is unavailable.", path.toString(), parentPath.toString()));
      }
      record.ParentId = record.Parent->getId();
    }
    // Distinct paths through parent aliases can refer to the same destination child name.
    if(!selectedPlacements[record.Parent.get()].insert(record.Owner->getName()).second)
    {
      return MakeErrorResult(-6203, fmt::format("Cannot publish '{}' from '{}': its destination placement is selected more than once. Select only one object named '{}' in that parent.",
                                                path.toString(), sourceFile.string(), record.Owner->getName()));
    }
    record.RemovalMessage = std::make_shared<DataRemovedMessage>(this, record.Id, record.Owner->getName());
    reservedNextId = std::max(reservedNextId, record.Id + 1);
    if(auto group = std::dynamic_pointer_cast<BaseGroup>(record.Owner); group != nullptr)
    {
      publicationGroups.emplace(path, std::move(group));
    }
    records.push_back(std::move(record));
  }
  return {};
}

bool DataStructure::rollbackImportPublication(std::vector<ImportPublicationRecord>& records, DataObject::IdType savedNextId, DataObject::IdType reservedNextId,
                                              [[maybe_unused]] ImportPublicationTestState* testState, [[maybe_unused]] bool allowRefusal)
{
  bool complete = true;
  // Finish all independent structural cleanup before any observer can change the destination.
  for(usize index = records.size(); index > 0; --index)
  {
    auto& record = records[index - 1];
    if(record.Owner == nullptr)
    {
      continue;
    }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(allowRefusal && testState != nullptr && testState->RefuseCleanup && index == 2 && record.Stage != ImportInsertionStage::None)
    {
      testState->CleanupConsumed = true;
      record.Cleanup = ImportCleanupStatus::Refused;
      complete = false;
      continue;
    }
#endif
    record.Cleanup = rollbackImportedObject(record);
    complete = complete && record.Cleanup == ImportCleanupStatus::Complete;
  }
  for(auto& record : std::views::reverse(records))
  {
    if(record.Cleanup == ImportCleanupStatus::Complete && record.NotifyRemoval && !record.NotificationAttempted)
    {
      record.NotificationAttempted = true;
      try
      {
        notify(record.RemovalMessage);
      } catch(...)
      {
        record.NotificationFailed = true;
      }
    }
  }
  for(auto& record : records)
  {
    if(record.Cleanup == ImportCleanupStatus::Complete)
    {
      record.Owner.reset();
      record.Parent.reset();
    }
  }
  // A removal observer can create unrelated objects. Preserve its advanced counter.
  if(complete && m_NextId == reservedNextId)
  {
    m_NextId = savedNextId;
  }
  return complete;
}

void DataStructure::appendImportCleanupDiagnostics(Result<>& result, const std::vector<ImportPublicationRecord>& records, const fs::path& sourceFile)
{
  for(const auto& record : records)
  {
    if(record.Cleanup != ImportCleanupStatus::Complete)
    {
      result.errors().push_back(
          {.code = -6215,
           .message = fmt::format("Import rollback retained '{}' (identifier {}) from '{}': {}. The destination ID counter remains advanced.", record.Path.toString(), record.Id, sourceFile.string(),
                                  record.Cleanup == ImportCleanupStatus::Children ? "selected children remain attached" : "exact placement cleanup did not complete")});
    }
    if(record.NotificationFailed)
    {
      result.errors().push_back({.code = -6216, .message = fmt::format("Import rollback removed '{}' (identifier {}), but a removal notification failed.", record.Path.toString(), record.Id)});
    }
  }
}

void DataStructure::retainImportFailure([[maybe_unused]] ImportPublicationTestState* testState, [[maybe_unused]] std::vector<ImportPublicationRecord>& records,
                                        [[maybe_unused]] DataObject::IdType savedNextId, [[maybe_unused]] DataObject::IdType reservedNextId)
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(testState != nullptr)
  {
    testState->Destination = this;
    testState->SavedNextId = savedNextId;
    testState->ReservedNextId = reservedNextId;
    testState->RetainedRecords = std::move(records);
  }
#endif
}

Result<> DataStructure::publishImportedObjects(DataStructure& prepared, const std::vector<DataPath>& paths, const fs::path& sourceFile, const ImportPublicationCheckpoint& checkpoint)
{
  return publishImportedObjectsImpl(prepared, paths, sourceFile, checkpoint, nullptr);
}

Result<> DataStructure::publishImportedObjectsImpl(DataStructure& prepared, const std::vector<DataPath>& paths, const fs::path& sourceFile, const ImportPublicationCheckpoint& checkpoint,
                                                   ImportPublicationTestState* testState)
{
  if(!checkpoint.belongsTo(*this))
  {
    return MakeErrorResult(
        -6217,
        fmt::format("Cannot publish selected objects from '{}': the publication checkpoint belongs to another destination. Capture a checkpoint from this destination before preparing the import.",
                    sourceFile.string()));
  }
  if(m_NextId != checkpoint.nextId())
  {
    return MakeErrorResult(
        -6218,
        fmt::format(
            "Cannot publish selected objects from '{}': the destination next identifier is {}, but the publication checkpoint recorded {}. Capture a new checkpoint and prepare the import again.",
            sourceFile.string(), m_NextId, checkpoint.nextId()));
  }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  using PublicationFaultType = ImportPublicationTestControl::Fault;
#endif
  const auto savedNextId = checkpoint.nextId();
  auto reservedNextId = savedNextId;
  std::vector<ImportPublicationRecord> records;
  auto importResult = prepareImportPublicationRecords(prepared, paths, sourceFile, records, reservedNextId);
  if(importResult.invalid() || records.empty())
  {
    return importResult;
  }

  // Publication owners, records, and removal messages are ready before the first destination mutation.
  m_NextId = reservedNextId;
  usize insertionIndex = 0;
  try
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(testState != nullptr && testState->Fault == PublicationFaultType::BeforePublication)
    {
      testState->Consumed = true;
      testState->Fault = PublicationFaultType::None;
      throw std::runtime_error("injected import failure before publication");
    }
#endif
    for(; insertionIndex < records.size(); ++insertionIndex)
    {
      auto failAfter = ImportInsertionStage::None;
      bool rejectBeforeInsert = false;
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      if(testState != nullptr && insertionIndex == 1)
      {
        testState->RetainedOwner = records[insertionIndex].Owner;
        if(testState->Fault == PublicationFaultType::AfterHierarchy)
        {
          failAfter = ImportInsertionStage::Hierarchy;
        }
        else if(testState->Fault == PublicationFaultType::BeforeTracking)
        {
          failAfter = ImportInsertionStage::Parent;
        }
        else if(testState->Fault == PublicationFaultType::ReturnFalse)
        {
          testState->Consumed = true;
          testState->Fault = PublicationFaultType::None;
          rejectBeforeInsert = true;
        }
      }
#endif
      if(!insertImportedObject(records[insertionIndex], failAfter, rejectBeforeInsert))
      {
        throw std::runtime_error("destination insertion rejected the prepared object");
      }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      if(testState != nullptr && insertionIndex == 1 && (testState->Fault == PublicationFaultType::AfterSecondInsert || testState->Fault == PublicationFaultType::AllocationAfterSecond))
      {
        const bool allocation = testState->Fault == PublicationFaultType::AllocationAfterSecond;
        testState->Fault = PublicationFaultType::None;
        testState->Consumed = true;
        if(allocation)
        {
          throw std::bad_alloc();
        }
        throw std::runtime_error("injected import failure after second insertion");
      }
#endif
    }
  } catch(const std::bad_alloc&)
  {
    rollbackImportPublication(records, savedNextId, reservedNextId, testState, true);
    retainImportFailure(testState, records, savedNextId, reservedNextId);
    throw;
  } catch(const std::exception& error)
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(testState != nullptr && insertionIndex < records.size() &&
       ((testState->Fault == PublicationFaultType::AfterHierarchy && records[insertionIndex].Stage == ImportInsertionStage::Hierarchy) ||
        (testState->Fault == PublicationFaultType::BeforeTracking && records[insertionIndex].Stage == ImportInsertionStage::Parent)))
    {
      testState->Consumed = true;
      testState->Fault = PublicationFaultType::None;
    }
#endif
    rollbackImportPublication(records, savedNextId, reservedNextId, testState, true);
    const auto& failed = records[std::min(insertionIndex, records.size() - 1)];
    importResult = MergeResults(std::move(importResult),
                                MakeErrorResult(-6214, fmt::format("Cannot publish '{}' (identifier {}) from '{}': {}", failed.Path.toString(), failed.Id, sourceFile.string(), error.what())));
    appendImportCleanupDiagnostics(importResult, records, sourceFile);
    retainImportFailure(testState, records, savedNextId, reservedNextId);
    return importResult;
  } catch(...)
  {
    rollbackImportPublication(records, savedNextId, reservedNextId, testState, true);
    importResult = MergeResults(std::move(importResult), MakeErrorResult(-6214, fmt::format("Cannot publish selected objects from '{}': an unknown insertion failure occurred.", sourceFile.string())));
    appendImportCleanupDiagnostics(importResult, records, sourceFile);
    retainImportFailure(testState, records, savedNextId, reservedNextId);
    return importResult;
  }
  return importResult;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
DataStructure::ImportPublicationTestControl::ImportPublicationTestControl() = default;
DataStructure::ImportPublicationTestControl::~ImportPublicationTestControl() noexcept = default;

void DataStructure::ImportPublicationTestControl::setFault(Fault fault, bool refuseCleanup)
{
  m_State = std::make_unique<ImportPublicationTestState>();
  m_State->Fault = fault;
  m_State->RefuseCleanup = refuseCleanup;
}

bool DataStructure::ImportPublicationTestControl::publicationFaultConsumed() const
{
  return m_State != nullptr && m_State->Consumed;
}

bool DataStructure::ImportPublicationTestControl::cleanupFaultConsumed() const
{
  return m_State != nullptr && m_State->CleanupConsumed;
}

std::shared_ptr<DataObject> DataStructure::ImportPublicationTestControl::retainedOwner() const
{
  return m_State == nullptr ? nullptr : m_State->RetainedOwner;
}

Result<> DataStructure::ImportPublicationTestControl::publish(DataStructure& destination, DataStructure& prepared, const std::vector<DataPath>& paths, const fs::path& sourceFile,
                                                              const ImportPublicationCheckpoint& checkpoint)
{
  return destination.publishImportedObjectsImpl(prepared, paths, sourceFile, checkpoint, m_State.get());
}

Result<> DataStructure::ImportPublicationTestControl::cleanup(DataStructure& destination)
{
  if(m_State == nullptr || m_State->Destination != &destination)
  {
    return MakeErrorResult(-6215, "The selected import has no retained rollback records for this destination.");
  }
  if(!destination.rollbackImportPublication(m_State->RetainedRecords, m_State->SavedNextId, m_State->ReservedNextId, m_State.get(), false))
  {
    return MakeErrorResult(-6215, "The selected import still has an exact-object cleanup conflict.");
  }
  return {};
}
#endif

std::shared_ptr<DataObject> DataStructure::makeImportPublicationCopy(DataObject& source)
{
  auto* associationPtr = source.m_DataStructure;
  source.DataObject::setDataStructure(nullptr);
  const auto restore = MakeScopeGuard([&source, associationPtr]() noexcept { source.DataObject::setDataStructure(associationPtr); });
  std::unique_ptr<DataObject> copy(source.shallowCopy());
  if(copy == nullptr)
  {
    return {};
  }
  if(auto* groupPtr = dynamic_cast<BaseGroup*>(copy.get()); groupPtr != nullptr)
  {
    groupPtr->clear();
  }
  copy->DataObject::setDataStructure(nullptr);
  copy->m_ParentList.clear();
  return {std::move(copy)};
}

bool DataStructure::insertImportedObject(ImportPublicationRecord& record, ImportInsertionStage failAfter, bool rejectBeforeInsert)
{
  if(rejectBeforeInsert || record.Owner == nullptr || record.Owner->getId() != record.Id || m_DataObjects.contains(record.Id))
  {
    return false;
  }
  if(record.Parent != nullptr && (record.Parent->getId() != record.ParentId || !record.Parent->acceptsChild(record.Owner.get())))
  {
    return false;
  }
  auto& map = record.Parent == nullptr ? m_RootGroup : record.Parent->getDataMap();
  if(map.find(record.Id) != map.end() || !map.insert(record.Owner))
  {
    return false;
  }
  record.Stage = ImportInsertionStage::Hierarchy;
  if(failAfter == record.Stage)
  {
    throw std::runtime_error("injected import failure after hierarchy placement");
  }
  if(record.Parent != nullptr)
  {
    record.Owner->addParent(record.Parent.get());
  }
  record.Stage = ImportInsertionStage::Parent;
  if(failAfter == record.Stage)
  {
    throw std::runtime_error("injected import failure before weak-index registration");
  }
  trackDataObject(record.Owner);
  record.Stage = ImportInsertionStage::Registered;
  return true;
}

DataStructure::ImportCleanupStatus DataStructure::rollbackImportedObject(ImportPublicationRecord& record) noexcept
{
  auto& object = *record.Owner;
  if(object.getId() != record.Id || (record.Parent != nullptr && record.Parent->getId() != record.ParentId))
  {
    return ImportCleanupStatus::Conflict;
  }
  auto& map = record.Parent == nullptr ? m_RootGroup : record.Parent->getDataMap();
  const auto placement = map.find(record.Id);
  if(placement != map.end() && placement->second.get() != &object)
  {
    return ImportCleanupStatus::Conflict;
  }
  for(const auto parentId : object.m_ParentList)
  {
    if(record.Parent == nullptr || parentId != record.ParentId)
    {
      return ImportCleanupStatus::Conflict;
    }
  }
  if(record.Parent != nullptr && m_RootGroup.find(record.Id) != m_RootGroup.end())
  {
    return ImportCleanupStatus::Conflict;
  }
  if(const auto* groupPtr = dynamic_cast<const BaseGroup*>(&object); groupPtr != nullptr && !groupPtr->empty())
  {
    return ImportCleanupStatus::Children;
  }
  const auto indexed = m_DataObjects.find(record.Id);
  if(indexed != m_DataObjects.end() && indexed->second.lock().get() != &object)
  {
    // A foreign weak entry must survive. A locally unowned shell must still leave scope without a stale callback.
    if(placement == map.end() && object.m_ParentList.empty())
    {
      object.DataObject::setDataStructure(nullptr);
    }
    return ImportCleanupStatus::Conflict;
  }
  if(placement != map.end())
  {
    map.erase(placement);
  }
  if(record.Parent != nullptr)
  {
    object.m_ParentList.remove(record.ParentId);
  }
  if(indexed != m_DataObjects.end())
  {
    m_DataObjects.erase(indexed);
  }
  record.NotifyRemoval = object.m_DataStructure == this;
  object.DataObject::setDataStructure(nullptr);
  return ImportCleanupStatus::Complete;
}

DataObject::IdType DataStructure::getNextId() const
{
  return m_NextId;
}

bool DataStructure::insertIntoRoot(const std::shared_ptr<DataObject>& dataObject)
{
  if(dataObject == nullptr)
  {
    return false;
  }

  if(!m_RootGroup.insert(dataObject))
  {
    return false;
  }
  trackDataObject(dataObject);
  return true;
}
bool DataStructure::insertIntoParent(const std::shared_ptr<DataObject>& dataObject, BaseGroup* parentGroup)
{
  if(parentGroup == nullptr)
  {
    return false;
  }

  if(!parentGroup->insert(dataObject))
  {
    return false;
  }
  trackDataObject(dataObject);
  return true;
}

void DataStructure::trackDataObject(const std::shared_ptr<DataObject>& dataObject)
{
  if(dataObject == nullptr)
  {
    return;
  }
  if(m_DataObjects.find(dataObject->getId()) == m_DataObjects.end())
  {
    m_DataObjects[dataObject->getId()] = dataObject;
    if(m_NextId <= dataObject->getId())
    {
      m_NextId = dataObject->getId() + 1;
    }
  }
  dataObject->setDataStructure(this);
}

bool DataStructure::setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParentId)
{
  auto& target = m_DataObjects[targetId];
  auto newParent = dynamic_cast<BaseGroup*>(getData(newParentId));
  if(newParent == nullptr)
  {
    return false;
  }

  if(!newParent->insert(target))
  {
    return false;
  }

  notify(std::make_shared<DataReparentedMessage>(this, targetId, newParentId, true));
  return true;
}

bool DataStructure::removeParent(DataObject::IdType targetId, DataObject::IdType parentId)
{
  const auto& target = m_DataObjects[targetId];
  const auto parent = dynamic_cast<BaseGroup*>(getData(parentId));
  const auto targetPtr = target.lock();
  if(targetPtr == nullptr)
  {
    return false;
  }
  if(parentId == 0)
  {
    return removeTopLevel(targetPtr.get());
  }
  return parent->remove(targetPtr.get());
}

DataStructure::SignalType& DataStructure::getSignal()
{
  return m_Signal;
}

void DataStructure::notify(const std::shared_ptr<AbstractDataStructureMessage>& msg)
{
  if(!m_IsValid || msg == nullptr)
  {
    return;
  }
  m_Signal(this, msg);
}

DataStructure& DataStructure::operator=(const DataStructure& rhs)
{
  m_DataObjects = rhs.m_DataObjects;
  m_RootGroup = rhs.m_RootGroup;
  m_IsValid = rhs.m_IsValid;
  m_NextId = rhs.m_NextId;
  m_FormatResolver = rhs.m_FormatResolver;

  // Keep copied objects alive while the root map rebinds their DataStructure.
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> sharedData;
  for(auto& [identifier, dataWkPtr] : rhs.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[identifier] = copy;
      m_DataObjects[identifier] = copy;
    }
  }
  applyAllDataStructure();
  return *this;
}

DataStructure& DataStructure::operator=(DataStructure&& rhs) noexcept
{
  m_DataObjects = std::move(rhs.m_DataObjects);
  m_RootGroup = std::move(rhs.m_RootGroup);
  m_IsValid = std::move(rhs.m_IsValid);
  m_NextId = std::move(rhs.m_NextId);
  m_FormatResolver = std::move(rhs.m_FormatResolver);

  applyAllDataStructure();
  return *this;
}

void DataStructure::applyAllDataStructure()
{
  m_RootGroup.setDataStructure(this);
}

nonstd::expected<void, std::string> DataStructure::validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const
{
  if(dataPaths.empty())
  {
    return {};
  }

  std::stringstream message;

  usize tupleCount = std::numeric_limits<usize>::max();
  for(const auto& dataPath : dataPaths)
  {
    auto* dataObject = getData(dataPath);

    const DataObject::Type dataObjectType = dataObject->getDataObjectType();
    size_t numTuples = 0;
    if(dataObjectType == DataObject::Type::NeighborList || dataObjectType == DataObject::Type::StringArray || dataObjectType == DataObject::Type::DataArray)
    {
      const auto* dataArrayPtr = getDataAs<IArray>(dataPath);
      numTuples = dataArrayPtr->getNumberOfTuples();
    }
    else
    {
      message << "Only NeighborList, StringArray and DataArray can be validated for tuple counts\n";
      return {nonstd::make_unexpected(message.str())};
    }

    auto parentPaths = dataObject->getDataPaths();
    for(const auto& path : parentPaths)
    {
      message << "DataPath: " << path.toString() << "    | Tuple Count: " << numTuples << "\n";
    }

    // The first array establishes the tuple count for all later comparisons.
    if(tupleCount == std::numeric_limits<usize>::max())
    {
      tupleCount = numTuples;
    }
    else if(tupleCount != numTuples)
    {
      return {nonstd::make_unexpected(message.str())};
    }
  }
  return {};
}

void DataStructure::resetIds(DataObject::IdType startingId)
{
  // Zero is the root identifier and cannot identify an inserted object.
  if(startingId == 0)
  {
    startingId = 1;
  }

  m_NextId = startingId;

  // Build an old-to-new map before updating hierarchy references.
  WeakCollectionType newCollection;
  std::unordered_map<DataObject::IdType, DataObject::IdType> updatedIdsMap;
  for(auto& dataObjectIter : m_DataObjects)
  {
    auto dataObjectPtr = dataObjectIter.second.lock();
    if(dataObjectPtr == nullptr)
    {
      continue;
    }

    auto oldId = dataObjectIter.first;
    auto newId = generateId();

    dataObjectPtr->setId(newId);
    updatedIdsMap[oldId] = newId;

    newCollection.insert({newId, dataObjectPtr});
  }

  m_DataObjects = newCollection;

  for(auto& dataObjectIter : m_DataObjects)
  {
    auto dataObjectPtr = dataObjectIter.second.lock();
    if(dataObjectPtr != nullptr)
    {
      dataObjectPtr->checkUpdatedIds(updatedIdsMap);
    }
  }
  m_RootGroup.updateIds(updatedIdsMap);
}

void DataStructure::exportHierarchyAsGraphViz(std::ostream& outputStream) const
{
  outputStream << "digraph DataGraph {\n"
               << "\tlabelloc =\"t\"\n"
               << "\trankdir=LR;\n"
               << "\tlabel=\"DataStructure Hierarchy\"\n"
               << "\tlabelloc=\"t\"\n"
               << "\tfontcolor=\"#FFFFFA\"\n"
               << "\tfontsize=12\n"
               << "\tgraph [splines=true bgcolor=\"#242627\"]\n"
               << "\tnode [shape=record style=\"filled\" fillcolor=\"#1D7ECD\" fontsize=12 fontcolor=\"#FFFFFA\"]\n"
               << "\tedge [dir=front arrowtail=empty style=\"\" color=\"#FFFFFA\"]\n\n";
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);
    outputStream << "\n/* Top level DataObject: " << topLevelPath.getTargetName() << " */\n\"" << topLevelPath.getTargetName() << "\";\n";

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      recurseHierarchyToGraphViz(outputStream, optionalDataPaths.value(), topLevelPath.getTargetName());
    }
  }

  outputStream << "}\n";
}

void DataStructure::exportHierarchyAsText(std::ostream& outputStream) const
{
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    outputStream << k_Delimiter << topLevelPath.getTargetName() << "\n";
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      recurseHierarchyToText(outputStream, optionalDataPaths.value(), "");
    }
  }
}

void DataStructure::recurseHierarchyToGraphViz(std::ostream& outputStream, const std::vector<DataPath> paths, const std::string& parent) const
{
  for(const auto& path : paths)
  {
    outputStream << "\"" << parent << "\" -> \"" << path.getTargetName() << "\"\n";

    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    recurseHierarchyToGraphViz(outputStream, optionalChildPaths.value(), path.getTargetName());
  }
  // outputStream << "\n"; // for readability
}

void DataStructure::recurseHierarchyToText(std::ostream& outputStream, const std::vector<DataPath> paths, std::string indent) const
{
  indent += "  ";

  for(const auto& path : paths)
  {
    outputStream << indent << k_Delimiter << path.getTargetName() << "\n";

    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    recurseHierarchyToText(outputStream, optionalChildPaths.value(), indent);
  }
}

void DataStructure::flush() const
{
  for(const auto& weakPtr : m_DataObjects)
  {
    std::shared_ptr<DataObject> sharedObj = weakPtr.second.lock();
    if(sharedObj == nullptr)
    {
      continue;
    }
    sharedObj->flush();
  }
}

Result<> DataStructure::flushChecked() const
{
  ErrorCollection errors;
  WarningCollection warnings;
  for(const auto& [id, weakObject] : m_DataObjects)
  {
    const auto object = weakObject.lock();
    if(object == nullptr)
    {
      continue;
    }

    Result<> objectResult;
    try
    {
      objectResult = object->flushChecked();
    } catch(const std::bad_alloc& error)
    {
      objectResult = MakeErrorResult(-272, fmt::format("Memory allocation failed during checked flush: {}", error.what()));
    } catch(const std::exception& error)
    {
      objectResult = MakeErrorResult(-6070, fmt::format("Checked flush failed: {}", error.what()));
    } catch(...)
    {
      objectResult = MakeErrorResult(-6070, "Checked flush failed: unknown storage failure.");
    }
    if(objectResult.invalid() && objectResult.errors().empty())
    {
      objectResult.errors().push_back({.code = -6070, .message = "Checked flush failed without an error diagnostic."});
    }
    if(objectResult.valid() && objectResult.warnings().empty())
    {
      continue;
    }

    // Clean objects avoid path construction. The ID index visits linked objects only once.
    std::string context = fmt::format("Checked flush for object '{}' (ID {})", object->getName(), id);
    const auto paths = object->getDataPaths();
    if(!paths.empty())
    {
      context += " at paths [";
      for(usize pathIdx = 0; pathIdx < paths.size(); ++pathIdx)
      {
        if(pathIdx != 0)
        {
          context += ", ";
        }
        context += fmt::format("'{}'", paths[pathIdx].toString());
      }
      context += "]";
    }

    // Normal vector growth avoids repeatedly reallocating all earlier diagnostics.
    if(objectResult.invalid())
    {
      for(auto& error : objectResult.errors())
      {
        error.message = fmt::format("{}: {}", context, error.message);
        errors.push_back(std::move(error));
      }
    }
    for(auto& warning : objectResult.warnings())
    {
      warning.message = fmt::format("{}: {}", context, warning.message);
      warnings.push_back(std::move(warning));
    }
    if(objectResult.invalid())
    {
      // Stop at the first failed object. Later objects keep their unflushed state for a later flush.
      break;
    }
  }

  Result<> result;
  if(!errors.empty())
  {
    result.m_Expected = nonstd::make_unexpected(std::move(errors));
  }
  result.warnings() = std::move(warnings);
  return result;
}

uint64 DataStructure::memoryUsage() const
{
  uint64 memory = 0;
  for(const auto& dataIter : m_DataObjects)
  {
    auto dataPtr = dataIter.second.lock();
    if(dataPtr == nullptr)
    {
      continue;
    }
    memory += dataPtr->memoryUsage();
  }
  return memory;
}

Result<> DataStructure::transferDataArraysOoc()
{
  Result<> result;

  // Resolve each array with the creation-time format resolver.
  // The resolver keeps core independent of concrete out-of-core formats.
  for(const auto& dataIter : m_DataObjects)
  {
    auto dataPtr = dataIter.second.lock();
    auto dataArrayPtr = std::dynamic_pointer_cast<IDataArray>(dataPtr);
    if(dataArrayPtr == nullptr)
    {
      continue;
    }

    // The first path supplies the geometry context. Multiple geometry links are
    // not a supported conversion configuration.
    const std::vector<DataPath> paths = getDataPathsForId(dataIter.first);
    if(paths.empty())
    {
      continue;
    }

    const std::string resolvedFormat = ArrayCreationUtilities::ResolveStorageFormat(*this, paths.front(), dataArrayPtr->getDataType(), dataArrayPtr->memoryUsage(), "");

    // An empty format selects in-core storage, so conversion is not required.
    if(resolvedFormat.empty())
    {
      continue;
    }

    if(!ConvertIDataArray(dataArrayPtr, resolvedFormat))
    {
      result.warnings().emplace_back(Warning{-3570, fmt::format("Cannot convert DataArray: '{}' to out-of-core", dataArrayPtr->getName())});
    }
  }

  return result;
}

Result<> DataStructure::validateGeometries() const
{
  // Geometry DataObject::Type values form a contiguous range. Unit tests assert
  // this range so a changed enumeration cannot silently skip validation.
  Result<> result;
  for(const auto& dataObject : m_RootGroup)
  {
    auto dataObjectType = dataObject.second->getDataObjectType();
    if(dataObjectType >= DataObject::Type::IGeometry && dataObjectType <= DataObject::Type::TetrahedralGeom)
    {
      auto* geomPtr = dynamic_cast<IGeometry*>(dataObject.second.get());
      result = MergeResults(geomPtr->validate(), result);
    }
  }
  return result;
}

Result<> DataStructure::validateAttributeMatrices() const
{
  Result<> result;
  for(const auto& dataObject : m_DataObjects)
  {
    auto dataObjectSharedPtr = dataObject.second.lock();
    if(dataObjectSharedPtr.get() != nullptr)
    {
      auto dataObjectType = dataObjectSharedPtr->getDataObjectType();
      if(dataObjectType == DataObject::Type::AttributeMatrix)
      {
        auto* attrMatPtr = dynamic_cast<AttributeMatrix*>(dataObject.second.lock().get());
        if(nullptr != attrMatPtr)
        {
          result = MergeResults(attrMatPtr->validate(), result);
        }
      }
    }
  }
  return result;
}

} // namespace nx::core
