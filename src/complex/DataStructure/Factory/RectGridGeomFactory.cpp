#include "RectGridGeomFactory.hpp"

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"

using namespace complex;

RectGridGeomFactory::RectGridGeomFactory()
: IH5DataFactory()
{
}

RectGridGeomFactory::~RectGridGeomFactory() = default;

std::string RectGridGeomFactory::getDataTypeName() const
{
  return "RectGridGeom";
}

H5::ErrorType RectGridGeomFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = RectGridGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}

//------------------------------------------------------------------------------
H5::ErrorType RectGridGeomFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
