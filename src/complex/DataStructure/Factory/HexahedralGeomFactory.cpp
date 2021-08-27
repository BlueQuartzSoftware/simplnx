#include "HexahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"

using namespace complex;

HexahedralGeomFactory::HexahedralGeomFactory()
: IH5DataFactory()
{
}

HexahedralGeomFactory::~HexahedralGeomFactory() = default;

std::string HexahedralGeomFactory::getDataTypeName() const
{
  return "HexahedralGeom";
}

H5::ErrorType HexahedralGeomFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = getObjName(targetId);
  auto geom = HexahedralGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}
