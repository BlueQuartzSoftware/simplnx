#include "RectGridGeomFactory.hpp"

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

RectGridGeomFactory::RectGridGeomFactory()
: IDataFactory()
{
}

RectGridGeomFactory::~RectGridGeomFactory() = default;

std::string RectGridGeomFactory::getDataTypeName() const
{
  return "RectGridGeom";
}

IDataFactory::error_type RectGridGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                   const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = RectGridGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
