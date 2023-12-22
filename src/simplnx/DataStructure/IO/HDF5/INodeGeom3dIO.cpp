#include "INodeGeom3dIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
INodeGeom3dIO::INodeGeom3dIO() = default;
INodeGeom3dIO::~INodeGeom3dIO() noexcept = default;

Result<> INodeGeom3dIO::ReadNodeGeom3dData(DataStructureReader& dataStructureReader, INodeGeometry3D& geom, const group_reader_type& parentGroup, const std::string& objectName,
                                           DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = INodeGeom2dIO::ReadNodeGeom2dData(dataStructureReader, geom, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  geom.setPolyhedronListId(ReadDataId(groupReader, IOConstants::k_PolyhedronListTag));
  geom.setPolyhedraDataId(ReadDataId(groupReader, IOConstants::k_PolyhedronDataTag));
  geom.setUnsharedFacedId(ReadDataId(groupReader, IOConstants::k_UnsharedFaceListTag));

  return {};
}
Result<> INodeGeom3dIO::WriteNodeGeom3dData(DataStructureWriter& dataStructureWriter, const INodeGeometry3D& geom, group_writer_type& parentGroup, bool importable)
{
  Result<> result = INodeGeom2dIO::WriteNodeGeom2dData(dataStructureWriter, geom, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroupWriter(geom.getName());
  result = WriteDataId(groupWriter, geom.getPolyhedronListId(), IOConstants::k_PolyhedronListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geom.getPolyhedraDataId(), IOConstants::k_PolyhedronDataTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geom.getUnsharedFacesId(), IOConstants::k_UnsharedFaceListTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
} // namespace nx::core::HDF5
