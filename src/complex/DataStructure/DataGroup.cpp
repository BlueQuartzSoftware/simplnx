#include "DataGroup.hpp"

#include <exception>
#include <stdexcept>

#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

namespace Constants
{
inline const std::string TypeName = "";
}

DataGroup::DataGroup(DataStructure& ds, const std::string& name)
: BaseGroup(ds, name)
{
}

DataGroup::DataGroup(DataStructure& ds, const std::string& name, IdType importId)
: BaseGroup(ds, name, importId)
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

DataGroup* DataGroup::Import(DataStructure& ds, const std::string& name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<DataGroup>(new DataGroup(ds, name, importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataObject* DataGroup::deepCopy()
{
  auto copy = new DataGroup(*getDataStructure(), getName());
  for(auto&[id, childPtr] : getDataMap())
  {
    copy->insert(childPtr);
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

H5::ErrorType DataGroup::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader)
{
  return BaseGroup::readHdf5(dataStructureReader, groupReader);
}

H5::ErrorType DataGroup::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  return BaseGroup::writeHdf5(dataStructureWriter, parentGroupWriter);
}
