#include "ImageGeomFactory.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

ImageGeomFactory::ImageGeomFactory()
: IH5DataFactory()
{
}

ImageGeomFactory::~ImageGeomFactory() = default;

std::string ImageGeomFactory::getDataTypeName() const
{
  return "ImageGeom";
}

H5::ErrorType ImageGeomFactory::createFromHdf5(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  auto name = getObjName(targetId);
  auto geom = ImageGeom::Create(ds, name, parentId);
  return geom->readHdf5(targetId, groupId);
}
