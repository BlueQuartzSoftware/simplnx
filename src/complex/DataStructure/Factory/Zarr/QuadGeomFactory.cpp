#include "QuadGeomFactory.hpp"

#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

QuadGeomFactory::QuadGeomFactory()
: IDataFactory()
{
}

QuadGeomFactory::~QuadGeomFactory() = default;

std::string QuadGeomFactory::getDataTypeName() const
{
  return "QuadGeom";
}

IDataFactory::error_type QuadGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                               const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  auto name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = QuadGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
