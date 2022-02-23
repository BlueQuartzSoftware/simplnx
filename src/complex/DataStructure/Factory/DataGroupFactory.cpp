#include "DataGroupFactory.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

DataGroupFactory::DataGroupFactory()
: IDataFactory()
{
}

DataGroupFactory::~DataGroupFactory() = default;

std::string DataGroupFactory::getDataTypeName() const
{
  return "DataGroup";
}

H5::ErrorType DataGroupFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                            const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto group = DataGroup::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return group->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType DataGroupFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                              const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
