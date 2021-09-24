#include "VertexGeomFactory.hpp"

#include "complex/DataStructure/Geometry/VertexGeom.hpp"

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

H5::ErrorType VertexGeomFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = VertexGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}

//------------------------------------------------------------------------------
H5::ErrorType VertexGeomFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
