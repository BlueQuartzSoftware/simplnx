#include "GridMontageFactory.hpp"

#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;
using namespace complex::H5;

GridMontageFactory::GridMontageFactory()
: IDataFactory()
{
}

GridMontageFactory::~GridMontageFactory() = default;

std::string GridMontageFactory::getDataTypeName() const
{
  return "GridMontage";
}

H5::ErrorType GridMontageFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                              const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  std::string name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto montage = GridMontage::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return montage->readHdf5(dataStructureReader, groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType GridMontageFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                                const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  return -1;
}
