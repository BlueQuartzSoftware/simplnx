#include "DataGroupFactory.hpp"

#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

DataGroupFactory::DataGroupFactory()
: IH5DataFactory()
{
}

DataGroupFactory::~DataGroupFactory() = default;

std::string DataGroupFactory::getDataTypeName() const
{
  return "DataGroup";
}

H5::ErrorType DataGroupFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto group = DataGroup::Create(ds, name, parentId);
  return group->readHdf5(targetId, groupId);
}

//------------------------------------------------------------------------------
H5::ErrorType DataGroupFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
