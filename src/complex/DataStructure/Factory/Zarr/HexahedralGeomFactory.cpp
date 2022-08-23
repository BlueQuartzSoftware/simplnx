#include "HexahedralGeomFactory.hpp"

#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

HexahedralGeomFactory::HexahedralGeomFactory()
: IDataFactory()
{
}

HexahedralGeomFactory::~HexahedralGeomFactory() = default;

std::string HexahedralGeomFactory::getDataTypeName() const
{
  return "HexahedralGeom";
}

IDataFactory::error_type HexahedralGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                     const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  std::string name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto geom = HexahedralGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader, preflight);
}
