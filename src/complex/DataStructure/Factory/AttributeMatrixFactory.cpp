#include "AttributeMatrixFactory.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

AttributeMatrixFactory::AttributeMatrixFactory()
: IDataFactory()
{
}

AttributeMatrixFactory::~AttributeMatrixFactory() = default;

std::string AttributeMatrixFactory::getDataTypeName() const
{
  return "AttributeMatrix";
}

H5::ErrorType AttributeMatrixFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                                  const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  auto name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto group = AttributeMatrix::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return group->readHdf5(dataStructureReader, groupReader, preflight);
}

//------------------------------------------------------------------------------
H5::ErrorType AttributeMatrixFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                                    const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}
