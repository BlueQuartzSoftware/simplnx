#include "DataStructure.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/DataStructure/Messaging/DataAddedMessage.hpp"
#include "complex/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "complex/DataStructure/Messaging/DataReparentedMessage.hpp"
#include "complex/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "complex/Filter/DataParameter.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <numeric>
#include <stdexcept>

#include <fmt/core.h>

namespace complex
{

namespace Constants
{
inline const std::string DataStructureTag = "DataStructure";
}

DataStructure::DataStructure()
: m_IsValid(true)
{
}

DataStructure::DataStructure(const DataStructure& ds)
: m_DataObjects(ds.m_DataObjects)
, m_RootGroup(ds.m_RootGroup)
, m_IsValid(ds.m_IsValid)
, m_NextId(ds.m_NextId)
{
  // Hold a shared_ptr copy of the DataObjects long enough for
  // m_RootGroup.setDataStructure(this) to operate.
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> sharedData;
  for(const auto& [id, dataWkPtr] : ds.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[id] = copy;
      m_DataObjects[id] = copy;
    }
  }
  // Updates all DataMaps with the corresponding m_DataObjects pointers.
  // Updates all DataObjects with their new DataStructure
  m_RootGroup.setDataStructure(this);
}

DataStructure::DataStructure(DataStructure&& ds) noexcept
: m_DataObjects(std::move(ds.m_DataObjects))
, m_RootGroup(std::move(ds.m_RootGroup))
, m_IsValid(ds.m_IsValid)
, m_NextId(ds.m_NextId)
{
  m_RootGroup.setDataStructure(this);
}

DataStructure::~DataStructure()
{
  m_IsValid = false;
  for(auto& [id, weakDataPtr] : m_DataObjects)
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

bool DataStructure::containsData(DataObject::IdType id) const
{
  return getData(id) != nullptr;
}

Result<LinkedPath> DataStructure::makePath(const DataPath& path)
{
  try
  {
    std::vector<DataObject::IdType> pathIds;
    std::string name = path[0];
    const DataObject* data = m_RootGroup[name];
    if(data == nullptr)
    {
      data = complex::DataGroup::Create(*this, name);
    }
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(usize i = 1; i < path.getLength(); i++)
    {
      name = path[i];
      data = (*parent)[name];
      if(data == nullptr)
      {
        data = DataGroup::Create(*this, name, pathIds.back());
      }
      pathIds.push_back(data->getId());

      parent = dynamic_cast<const BaseGroup*>(data);
    }

    return {LinkedPath(this, pathIds)};
  } catch(const std::exception& e)
  {
    return complex::MakeErrorResult<LinkedPath>(-2, "Exception thrown when attempting to create a path in the DataStructure");
  }
}

std::vector<DataPath> DataStructure::getAllDataPaths() const
{
  std::vector<DataPath> dataPaths;
  for(const auto& [id, weakPtr] : m_DataObjects)
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
  for(const auto& [id, weakPtr] : m_DataObjects)
  {
    dataIds.push_back(id);
  }
  return dataIds;
}

DataObject* DataStructure::getData(DataObject::IdType id)
{
  auto iter = m_DataObjects.find(id);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& id)
{
  if(!id)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(id.value());
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

DataObject* DataStructure::getData(const LinkedPath& path)
{
  return getData(path.getId());
}

const DataObject* DataStructure::getData(DataObject::IdType id) const
{
  auto iter = m_DataObjects.find(id);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

const DataObject* DataStructure::getData(const std::optional<DataObject::IdType>& id) const
{
  if(!id)
  {
    return nullptr;
  }

  auto iter = m_DataObjects.find(id.value());
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

const DataObject* DataStructure::getData(const LinkedPath& path) const
{
  return path.getData();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(DataObject::IdType id) const
{
  if(m_DataObjects.find(id) == m_DataObjects.end())
  {
    return nullptr;
  }
  return m_DataObjects.at(id).lock();
}

std::shared_ptr<DataObject> DataStructure::getSharedData(const DataPath& path) const
{
  auto dataObject = getData(path);
  if(dataObject == nullptr)
  {
    return nullptr;
  }
  return m_DataObjects.at(dataObject->getId()).lock();
}

bool DataStructure::removeData(DataObject::IdType id)
{
  DataObject* data = getData(id);
  return removeData(data);
}

void DataStructure::setData(DataObject::IdType id, std::shared_ptr<DataObject> dataObject)
{
  if(dataObject == nullptr)
  {
    removeData(id);
    return;
  }

  m_DataObjects[id] = dataObject;
}

bool DataStructure::removeData(const std::optional<DataObject::IdType>& id)
{
  if(!id)
  {
    return false;
  }
  else
  {
    return removeData(id.value());
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

void DataStructure::dataDeleted(DataObject::IdType id, const std::string& name)
{
  if(!m_IsValid)
  {
    return;
  }

  auto msg = std::make_shared<DataRemovedMessage>(this, id, name);
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
  auto parent = dynamic_cast<BaseGroup*>(getData(parentId));
  auto targetPtr = target.lock();
  if(targetPtr == nullptr)
  {
    return false;
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
  for(auto& [id, dataWkPtr] : rhs.m_DataObjects)
  {
    auto dataPtr = dataWkPtr.lock();
    if(dataPtr != nullptr)
    {
      auto copy = std::shared_ptr<DataObject>(dataPtr->shallowCopy());
      sharedData[id] = copy;
      m_DataObjects[id] = copy;
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

H5::ErrorType DataStructure::writeHdf5(H5::GroupWriter& parentGroupWriter) const
{
  H5::DataStructureWriter dataStructureWriter;
  auto groupWriter = parentGroupWriter.createGroupWriter(Constants::k_DataStructureTag);
  auto idAttribute = groupWriter.createAttribute(Constants::k_NextIdTag);
  H5::ErrorType err = idAttribute.writeValue(m_NextId);
  if(err < 0)
  {
    return err;
  }

  err = m_RootGroup.writeH5Group(dataStructureWriter, groupWriter);
  return err;
}

DataStructure DataStructure::readFromHdf5(const H5::GroupReader& groupReader, H5::ErrorType& err)
{
  H5::DataStructureReader dataStructureReader;
  return dataStructureReader.readH5Group(groupReader, err);
}

bool DataStructure::validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const
{
  if(dataPaths.empty())
  {
    return true;
  }

  usize tupleCount = 0;
  for(usize i = 0; i < dataPaths.size(); i++)
  {
    const auto& dataPath = dataPaths[i];
    auto* dataArray = getDataAs<IDataArray>(dataPath);
    // Must be an IDataArray
    if(dataArray == nullptr)
    {
      return false;
    }
    // Check equality if not first item
    if(i == 0)
    {
      tupleCount = dataArray->getNumberOfTuples();
    }
    else if(tupleCount != dataArray->getNumberOfTuples())
    {
      return false;
    }
  }
  return true;
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
    dataObjectPtr->checkUpdatedIds(updatedIds);
  }
}

} // namespace complex
