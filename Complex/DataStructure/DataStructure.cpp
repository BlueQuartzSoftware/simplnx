#include "DataStructure.h"

#include "SIMPL/DataStructure/BaseGroup.h"
#include "SIMPL/DataStructure/DataGroup.h"
#include "SIMPL/DataStructure/LinkedPath.h"
#include "SIMPL/DataStructure/Messaging/DataAddedMessage.h"
#include "SIMPL/DataStructure/Messaging/DataRemovedMessage.h"
#include "SIMPL/DataStructure/Messaging/DataReparentedMessage.h"
#include "SIMPL/DataStructure/Observers/AbstractDataStructureObserver.h"

using namespace SIMPL;

DataStructure::DataStructure()
: m_IsValid(true)
{
}

DataStructure::DataStructure(const DataStructure& ds)
: m_DataObjects(ds.m_DataObjects)
, m_RootGroup(ds.m_RootGroup)
, m_IsValid(ds.m_IsValid)
{
  std::map<DataObject::IdType, std::shared_ptr<DataObject>> m_CopyData;
  for(auto& dataPair : m_DataObjects)
  {
    auto id = dataPair.first;
    auto copy = std::shared_ptr<DataObject>(dataPair.second.lock()->shallowCopy());
    m_CopyData[id] = copy;
    m_DataObjects[id] = copy;
  }
  m_RootGroup.setDataStructure(this);
}

DataStructure::DataStructure(DataStructure&& ds) noexcept
: m_DataObjects(std::move(ds.m_DataObjects))
, m_RootGroup(std::move(ds.m_RootGroup))
, m_IsValid(std::move(ds.m_IsValid))
{
  m_RootGroup.setDataStructure(this);
}

DataStructure::~DataStructure()
{
  m_Observers.clear();
  m_IsValid = false;
}

size_t DataStructure::size() const
{
  return m_DataObjects.size();
}

std::optional<DataObject::IdType> DataStructure::getId(const DataPath& path) const
{
  return getData(path)->getId();
}

LinkedPath DataStructure::getLinkedPath(const DataPath& path) const
{
  try
  {
    std::vector<size_t> pathIds;
    const DataObject* data = m_RootGroup[path[0]];
    const BaseGroup* parent = dynamic_cast<const BaseGroup*>(data);
    pathIds.push_back(data->getId());

    for(size_t i = 1; i < path.getLength(); i++)
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

DataObject* DataStructure::getData(DataObject::IdType id)
{
  auto iter = m_DataObjects.find(id);
  if(m_DataObjects.end() == iter)
  {
    return nullptr;
  }
  return iter->second.lock().get();
}

DataObject* traversePath(DataObject* obj, const DataPath& path, size_t index)
{
  if(path.getLength() == index)
  {
    return obj;
  }
  auto col = dynamic_cast<BaseGroup*>(obj);
  if(nullptr == col)
  {
    return nullptr;
  }
  DataObject* child = (*col)[path[index]];
  return traversePath(child, path, index + 1);
}

DataObject* DataStructure::getData(const DataPath& path)
{
  auto topLevel = getTopLevelData();
  for(DataObject* obj : topLevel)
  {
    if(nullptr == obj)
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

const DataObject* DataStructure::getData(const DataPath& path) const
{
  auto topLevel = getTopLevelData();
  for(DataObject* obj : topLevel)
  {
    if(nullptr == obj)
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

bool DataStructure::removeData(DataObject::IdType id)
{
  DataObject* data = getData(id);
  return removeData(data);
}

bool DataStructure::removeData(const DataPath& path)
{
  DataObject* data = getData(path);
  return removeData(data);
}

bool DataStructure::removeData(DataObject* data)
{
  if(nullptr == data)
  {
    return false;
  }

  auto pathsToData = data->getDataPaths();
  auto parents = data->getParents();
  if(parents.size() == 0)
  {
    return removeTopLevel(data);
  }
  for(BaseGroup* parent : parents)
  {
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

  m_DataObjects.erase(id);
  auto msg = std::make_shared<DataRemovedMessage>(this, id, name);
  notify(msg);
}

std::vector<DataObject*> DataStructure::getTopLevelData() const
{
  std::vector<DataObject*> topLevel;
  for(auto iter : m_DataObjects)
  {
    auto obj = iter.second.lock();
    if(nullptr == obj)
    {
      continue;
    }
    if(obj->getParents().size() == 0)
    {
      topLevel.push_back(obj.get());
    }
  }

  return topLevel;
}

bool DataStructure::insertTopLevel(const std::shared_ptr<BaseGroup>& container)
{
  if(nullptr == container)
  {
    return false;
  }

  return m_RootGroup.insert(container);
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

DataGroup* DataStructure::createGroup(const std::string& name, std::optional<DataObject::IdType> parent)
{
  auto container = std::make_shared<DataGroup>(this, name);
  auto weakPtr = std::weak_ptr<DataGroup>(container);
  if(parent.has_value())
  {
    auto parentContainer = dynamic_cast<BaseGroup*>(getData(parent.value()));
    if(!parentContainer->insert(weakPtr))
    {
      return nullptr;
    }
  }
  else if(!insertTopLevel(container))
  {
    return nullptr;
  }

  m_DataObjects[container->getId()] = weakPtr;
  notify(std::make_shared<DataAddedMessage>(this, container->getId()));
  return container.get();
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

bool DataStructure::setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParentId)
{
  auto target = m_DataObjects[targetId];
  auto newParent = dynamic_cast<BaseGroup*>(getData(newParentId));
  if(nullptr == newParent)
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
  auto target = m_DataObjects[targetId];
  auto parent = dynamic_cast<BaseGroup*>(getData(parentId));
  auto targetPtr = target.lock();
  if(targetPtr == nullptr)
  {
    return false;
  }
  return parent->remove(targetPtr.get());
}

void DataStructure::notify(const std::shared_ptr<AbstractDataStructureMessage>& msg)
{
  for(auto observer : m_Observers)
  {
    observer->onNotify(this, msg);
  }
}

void DataStructure::addObserver(AbstractDataStructureObserver* obs)
{
  m_Observers.insert(obs);
}

void DataStructure::removeObserver(AbstractDataStructureObserver* obs)
{
  m_Observers.erase(obs);
}

H5::ErrorType DataStructure::writeXdmfFile(const std::filesystem::path& hdfFilePath) const
{
  throw std::exception();
}

H5::ErrorType DataStructure::readXdmfFile(const std::filesystem::path& hdfFilePath)
{
  throw std::exception();
}
