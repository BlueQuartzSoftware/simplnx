#include "ImageGeomFactory.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

ImageGeomFactory::ImageGeomFactory()
: Zarr::IDataFactory()
{
}

ImageGeomFactory::~ImageGeomFactory() = default;

std::string ImageGeomFactory::getDataTypeName() const
{
  return "ImageGeom";
}

IDataFactory::error_type ImageGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = ImageGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
