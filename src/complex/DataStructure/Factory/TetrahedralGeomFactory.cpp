#include "TetrahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TetrahedralGeomFactory::TetrahedralGeomFactory()
: IH5DataFactory()
{
}

TetrahedralGeomFactory::~TetrahedralGeomFactory() = default;

std::string TetrahedralGeomFactory::getDataTypeName() const
{
  return "TetrahedralGeom";
}

H5::ErrorType TetrahedralGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = TetrahedralGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType TetrahedralGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
