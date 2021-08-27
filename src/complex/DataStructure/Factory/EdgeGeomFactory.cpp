#include "EdgeGeomFactory.hpp"

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"

using namespace complex;

EdgeGeomFactory::EdgeGeomFactory()
: IH5DataFactory()
{
}

EdgeGeomFactory::~EdgeGeomFactory() = default;

std::string EdgeGeomFactory::getDataTypeName() const
{
  return "EdgeGeom";
}

H5::ErrorType EdgeGeomFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = getObjName(targetId);
  auto geom = EdgeGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}
