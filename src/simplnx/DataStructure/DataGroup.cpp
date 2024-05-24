#include "DataGroup.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
using namespace nx::core;

DataGroup::DataGroup(DataStructure& dataStructure, std::string name)
: BaseGroup(dataStructure, std::move(name))
{
}

DataGroup::DataGroup(DataStructure& dataStructure, std::string name, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
{
}

DataGroup::DataGroup(const DataGroup& other)
: BaseGroup(other)
{
}

DataGroup::DataGroup(DataGroup&& other)
: BaseGroup(std::move(other))
{
}

DataGroup::~DataGroup() = default;

DataObject::Type DataGroup::getDataObjectType() const
{
  return Type::DataGroup;
}

BaseGroup::GroupType DataGroup::getGroupType() const
{
  return GroupType::DataGroup;
}

DataGroup* DataGroup::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataGroup* DataGroup::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::shared_ptr<DataObject> DataGroup::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  const auto copy = std::shared_ptr<DataGroup>(new DataGroup(dataStruct, copyPath.getTargetName()));
  copy->clear();
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);
    return copy;
  }
  return nullptr;
}

DataObject* DataGroup::shallowCopy()
{
  return new DataGroup(*this);
}

std::string DataGroup::getTypeName() const
{
  return k_TypeName;
}

Result<bool> DataGroup::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}
