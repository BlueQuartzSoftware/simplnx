#include "DataGroup.hpp"

#include <exception>
#include <stdexcept>

#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

using namespace complex;

namespace Constants
{
inline const std::string TypeName = "";
}

DataGroup::DataGroup(DataStructure& ds, const std::string& name)
: BaseGroup(ds, name)
{
}

DataGroup::DataGroup(const DataGroup& other)
: BaseGroup(other)
{
}

DataGroup::DataGroup(DataGroup&& other) noexcept
: BaseGroup(std::move(other))
{
}

DataGroup::~DataGroup() = default;

DataGroup* DataGroup::Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(ds, name));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataObject* DataGroup::deepCopy()
{
  auto copy = new DataGroup(*getDataStructure(), getName());
  for(auto& pair : getDataMap())
  {
    copy->insert(pair.second);
  }
  return copy;
}

DataObject* DataGroup::shallowCopy()
{
  return new DataGroup(*this);
}

std::string DataGroup::getTypeName() const
{
  return "DataGroup";
}

bool DataGroup::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}

H5::ErrorType DataGroup::readHdf5(H5::IdType targetId, H5::IdType parentId)
{
  return BaseGroup::readHdf5(targetId, parentId);
}

H5::ErrorType DataGroup::writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const
{
  return BaseGroup::writeHdf5_impl(parentId, groupId);
}
