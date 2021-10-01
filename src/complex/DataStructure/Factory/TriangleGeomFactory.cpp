#include "TriangleGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TriangleGeomFactory::TriangleGeomFactory()
: IH5DataFactory()
{
}

TriangleGeomFactory::~TriangleGeomFactory() = default;

std::string TriangleGeomFactory::getDataTypeName() const
{
  return "TriangleGeom";
}

H5::ErrorType TriangleGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = TriangleGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType TriangleGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
