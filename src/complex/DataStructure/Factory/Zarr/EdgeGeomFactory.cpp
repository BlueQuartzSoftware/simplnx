#include "EdgeGeomFactory.hpp"

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

EdgeGeomFactory::EdgeGeomFactory()
: IDataFactory()
{
}

EdgeGeomFactory::~EdgeGeomFactory() = default;

std::string EdgeGeomFactory::getDataTypeName() const
{
  return "EdgeGeom";
}

IDataFactory::error_type EdgeGeomFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                               const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::IGroup&>(baseReader);
  std::string name = groupReader.name();
  auto importId = ReadObjectId(baseReader);
  auto geom = EdgeGeom::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return geom->readZarr(dataStructureReader, groupReader);
}
