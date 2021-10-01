#include "DataGroupFactory.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

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

H5::ErrorType DataGroupFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto group = DataGroup::Create(ds, name, parentId);
  return group->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType DataGroupFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
