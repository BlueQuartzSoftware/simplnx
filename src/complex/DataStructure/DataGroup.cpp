#include "DataGroup.hpp"

#include <exception>
#include <stdexcept>

#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

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
  return "DataGroup";
}

bool DataGroup::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}

H5::ErrorType DataGroup::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType DataGroup::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  return BaseGroup::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
}
