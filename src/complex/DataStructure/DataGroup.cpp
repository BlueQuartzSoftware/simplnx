#include "DataGroup.hpp"

#include <exception>
#include <stdexcept>

using namespace complex;

DataGroup::DataGroup(DataStructure& ds, std::string name)
: BaseGroup(ds, std::move(name))
{
}

DataGroup::DataGroup(DataStructure& ds, std::string name, IdType importId)
: BaseGroup(ds, std::move(name), importId)
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

DataGroup* DataGroup::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataGroup* DataGroup::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataObject* DataGroup::deepCopy()
{
  return new DataGroup(*this);
}

DataObject* DataGroup::shallowCopy()
{
  return new DataGroup(*this);
}

std::string DataGroup::getTypeName() const
{
  return GetTypeName();
}

std::string DataGroup::GetTypeName()
{
  return "DataGroup";
}

bool DataGroup::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}
