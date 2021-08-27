#include "QuadGeomFactory.hpp"

#include "complex/DataStructure/Geometry/QuadGeom.hpp"

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

H5::ErrorType QuadGeomFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = QuadGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}
