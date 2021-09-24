#include "TriangleGeomFactory.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"

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

H5::ErrorType TriangleGeomFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = TriangleGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}

//------------------------------------------------------------------------------
H5::ErrorType TriangleGeomFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
