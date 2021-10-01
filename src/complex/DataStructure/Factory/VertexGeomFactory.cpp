#include "VertexGeomFactory.hpp"

#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

VertexGeomFactory::VertexGeomFactory()
: IH5DataFactory()
{
}

VertexGeomFactory::~VertexGeomFactory() = default;

std::string VertexGeomFactory::getDataTypeName() const
{
  return "VertexGeom";
}

H5::ErrorType VertexGeomFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  auto name = groupReader.getName();
  auto geom = VertexGeom::Create(ds, name, parentId);
  return geom->readHdf5(groupReader);
}

//------------------------------------------------------------------------------
H5::ErrorType VertexGeomFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
