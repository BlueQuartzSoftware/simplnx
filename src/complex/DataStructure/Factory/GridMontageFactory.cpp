#include "GridMontageFactory.hpp"

#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

GridMontageFactory::GridMontageFactory()
: IH5DataFactory()
{
}

GridMontageFactory::~GridMontageFactory() = default;

std::string GridMontageFactory::getDataTypeName() const
{
  return "GridMontage";
}

H5::ErrorType GridMontageFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto montage = GridMontage::Create(ds, name, parentId);
  return montage->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType GridMontageFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
