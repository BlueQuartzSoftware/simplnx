#include "DataStructure.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataReparentedMessage.hpp"
#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/core.h>

#include <numeric>
#include <sstream>
#include <stdexcept>

namespace
{
inline const std::string k_Delimiter = "|--";
}

namespace nx::core
{

namespace Constants
{
inline const std::string DataStructureTag = "DataStructure";
}

DataStructure::DataStructure()
: m_IsValid(true)
{
}

DataStructure::DataStructure(const DataStructure& dataStructure)
: m_DataObjects(dataStructure.m_DataObjects)
, m_RootGroup(dataStructure.m_RootGroup)
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
{
  // Hold a shared_ptr copy of the DataObjects long enough for
  // m_RootGroup.setDataStructure(this) to operate.
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
  // Updates all DataMaps with the corresponding m_DataObjects pointers.
  // Updates all DataObjects with their new DataStructure
  m_RootGroup.setDataStructure(this);
}

DataStructure::DataStructure(DataStructure&& dataStructure) noexcept
: m_DataObjects(std::move(dataStructure.m_DataObjects))
, m_RootGroup(std::move(dataStructure.m_RootGroup))
, m_IsValid(dataStructure.m_IsValid)
, m_NextId(dataStructure.m_NextId)
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
  } catch(std::exception e)
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

DataObject* traversePath(DataObject* obj, const DataPath& path, usize index)
{
  if(path.getLength() == index)
  {
    return obj;
  }
  auto col = dynamic_cast<BaseGroup*>(obj);
  if(col == nullptr)
  {
    return nullptr;
  }
  DataObject* child = (*col)[path[index]];
  return traversePath(child, path, index + 1);
}

DataObject* DataStructure::getData(const DataPath& path)
{
  if(path.empty())
  {
    return nullptr;
  }
  auto topLevel = getTopLevelData();
  for(DataObject* obj : topLevel)
  {
    if(obj == nullptr)
    {
      continue;
    }
    if(obj->getName() == path[0])
    {
      return traversePath(obj, path, 1);
    }
  }
  return nullptr;
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

  auto topLevel = getTopLevelData();
  for(DataObject* obj : topLevel)
  {
    if(obj == nullptr)
    {
      continue;
    }
    if(obj->getName() == path[0])
    {
      return traversePath(obj, path, 1);
    }
  }
  return nullptr;
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
  std::vector<DataObject*> topLevel;
  for(auto& iter : m_RootGroup)
  {
    auto obj = iter.second;
    topLevel.push_back(obj.get());
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

  // Clears the DataObject's parent IDs to avoid clashes.
  dataObject->clearParents();

  if(dataPath.empty())
  {
    return insertIntoRoot(dataObject);
  }

  auto parentGroup = getDataAs<BaseGroup>(dataPath);
  return insertIntoParent(dataObject, parentGroup);
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

  // Hold a shared_ptr copy of the DataObjects long enough for
  // m_RootGroup.setDataStructure(this) to operate.
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
  // Updates all DataMaps with the corresponding m_DataObjects pointers.
  // Updates all DataObjects with their new DataStructure
  applyAllDataStructure();
  return *this;
}

DataStructure& DataStructure::operator=(DataStructure&& rhs) noexcept
{
  m_DataObjects = std::move(rhs.m_DataObjects);
  m_RootGroup = std::move(rhs.m_RootGroup);
  m_IsValid = std::move(rhs.m_IsValid);
  m_NextId = std::move(rhs.m_NextId);

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
    else // We can only check DataObject subclasses that hold items that can be expressed as getNumberOfTuples();
    {
      message << "Only NeighborList, StringArray and DataArray can be validated for tuple counts\n";
      return {nonstd::make_unexpected(message.str())};
    }

    auto parentPaths = dataObject->getDataPaths();
    for(const auto& path : parentPaths)
    {
      message << "DataPath: " << path.toString() << "    | Tuple Count: " << numTuples << "\n";
    }

    // Check equality if not first item
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
  // 0 is reserved
  if(startingId == 0)
  {
    startingId = 1;
  }

  m_NextId = startingId;

  // Update DataObject IDs and track changes
  WeakCollectionType newCollection;
  using UpdatedId = std::pair<DataObject::IdType, DataObject::IdType>;
  std::vector<UpdatedId> updatedIds;
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
    updatedIds.push_back({oldId, newId});

    newCollection.insert({newId, dataObjectPtr});
  }

  // Update m_DataObjects collection
  m_DataObjects = newCollection;

  // Update ID references between DataObjects
  for(auto& dataObjectIter : m_DataObjects)
  {
    auto dataObjectPtr = dataObjectIter.second.lock();
    if(dataObjectPtr != nullptr)
    {
      dataObjectPtr->checkUpdatedIds(updatedIds);
    }
  }
  m_RootGroup.updateIds(updatedIds);
}

void DataStructure::exportHierarchyAsGraphViz(std::ostream& outputStream) const
{
  // initialize dot file
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
  // set base case
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);
    outputStream << "\n/* Top level DataObject: " << topLevelPath.getTargetName() << " */\n\"" << topLevelPath.getTargetName() << "\";\n";

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      // Begin recursion
      recurseHierarchyToGraphViz(outputStream, optionalDataPaths.value(), topLevelPath.getTargetName());
    }
  }

  // close dot file
  outputStream << "}\n"; // for readability
}

void DataStructure::exportHierarchyAsText(std::ostream& outputStream) const
{
  // set base case
  for(const auto* object : getTopLevelData())
  {
    auto topLevelPath = DataPath::FromString(object->getDataPaths()[0].getTargetName()).value();
    outputStream << k_Delimiter << topLevelPath.getTargetName() << "\n";
    auto optionalDataPaths = GetAllChildDataPaths(*this, topLevelPath);

    if(optionalDataPaths.has_value() && !optionalDataPaths.value().empty())
    {
      // Begin recursion
      recurseHierarchyToText(outputStream, optionalDataPaths.value(), "");
    }
  }
}

void DataStructure::recurseHierarchyToGraphViz(std::ostream& outputStream, const std::vector<DataPath> paths, const std::string& parent) const
{
  for(const auto& path : paths)
  {
    auto* dataObjectPtr = getData(path);
    // Output parent node, child node, and edge connecting them in .dot format
    outputStream << "\"" << parent << "\" -> \"" << path.getTargetName() << "\"\n";

    // pull child paths or skip to next iteration
    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    // recurse
    recurseHierarchyToGraphViz(outputStream, optionalChildPaths.value(), path.getTargetName());
  }
  // outputStream << "\n"; // for readability
}

void DataStructure::recurseHierarchyToText(std::ostream& outputStream, const std::vector<DataPath> paths, std::string indent) const
{
  indent += "  ";

  for(const auto& path : paths)
  {
    // Output parent node, child node, and edge connecting them in .dot format
    outputStream << indent << k_Delimiter << path.getTargetName() << "\n";

    // pull child paths or skip to next iteration
    auto optionalChildPaths = GetAllChildDataPaths(*this, path);
    if(!optionalChildPaths.has_value() || optionalChildPaths.value().empty())
    {
      continue;
    }

    // recurse
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
  auto* preferences = Application::GetOrCreateInstance()->getPreferences();
  if(!preferences->useOocData())
  {
    return MakeErrorResult(-3567, "Out-of-core not available");
  }

  Result<> result;
  std::string targetFormat = preferences->largeDataFormat();
  for(const auto& dataIter : m_DataObjects)
  {
    auto dataPtr = dataIter.second.lock();
    auto dataArrayPtr = std::dynamic_pointer_cast<IDataArray>(dataPtr);
    if(dataArrayPtr == nullptr)
    {
      continue;
    }
    if(!ConvertIDataArray(dataArrayPtr, targetFormat))
    {
      result.warnings().emplace_back(Warning{-3570, fmt::format("Cannot convert DataArray: '{}' to out-of-core", dataArrayPtr->getName())});
    }
  }

  return result;
}
} // namespace nx::core
