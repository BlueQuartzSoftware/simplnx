#include "QuadGeomFactory.hpp"

#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

QuadGeomFactory::QuadGeomFactory()
: IH5DataFactory()
{
}

QuadGeomFactory::~QuadGeomFactory() = default;

std::string QuadGeomFactory::getDataTypeName() const
{
  return "QuadGeom";
}

H5::ErrorType QuadGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = QuadGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType QuadGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
