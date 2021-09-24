#include "GridMontageFactory.hpp"

#include "complex/DataStructure/Montage/GridMontage.hpp"

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

H5::ErrorType GridMontageFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = getObjName(targetId);
  auto montage = GridMontage::Create(ds, name, parentId);
  return montage->readHdf5(targetId, groupId);
}

//------------------------------------------------------------------------------
H5::ErrorType GridMontageFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
