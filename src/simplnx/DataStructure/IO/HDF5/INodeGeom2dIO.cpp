#include "INodeGeom2dIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
INodeGeom2dIO::INodeGeom2dIO() = default;
INodeGeom2dIO::~INodeGeom2dIO() noexcept = default;

Result<> INodeGeom2dIO::ReadNodeGeom2dData(DataStructureReader& dataStructureReader, INodeGeometry2D& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                           DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = INodeGeom1dIO::ReadNodeGeom1dData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  geometry.setFaceListId(ReadDataId(groupReader, IOConstants::k_FaceListTag));
  geometry.setFaceDataId(ReadDataId(groupReader, IOConstants::k_FaceDataTag));
  geometry.setUnsharedEdgesId(ReadDataId(groupReader, IOConstants::k_UnsharedEdgeListTag));

  return {};
}

Result<> INodeGeom2dIO::WriteNodeGeom2dData(DataStructureWriter& dataStructureWriter, const INodeGeometry2D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = INodeGeom1dIO::WriteNodeGeom1dData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  nx::core::HDF5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(geometry.getName());
  result = WriteDataId(groupWriter, geometry.getFaceListId(), IOConstants::k_FaceListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getFaceAttributeMatrixId(), IOConstants::k_FaceDataTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getUnsharedEdgesId(), IOConstants::k_UnsharedEdgeListTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
} // namespace nx::core::HDF5
