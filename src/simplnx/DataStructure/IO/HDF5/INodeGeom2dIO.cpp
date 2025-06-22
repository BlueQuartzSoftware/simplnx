#include "INodeGeom2dIO.hpp"

#include "DataStructureReader.hpp"
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

  // Required data
  dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_FaceListTag));
  dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_FaceDataTag));
  dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_UnsharedEdgeListTag));

  return {};
}

Result<> INodeGeom2dIO::FinishImportingNodeGeom2dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<INodeGeometry2D>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50591, fmt::format("Failed to finish importing INodeGeometry2D at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  {
    auto groupReader = dataStructureGroup.openGroup(dataPath.toString());
    geom->setFaceListId(ReadDataId(groupReader, IOConstants::k_FaceListTag));
    geom->setFaceDataId(ReadDataId(groupReader, IOConstants::k_FaceDataTag));
    geom->setUnsharedEdgesId(ReadDataId(groupReader, IOConstants::k_UnsharedEdgeListTag));
  }

  return INodeGeom1dIO::FinishImportingNodeGeom1dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> INodeGeom2dIO::WriteNodeGeom2dData(DataStructureWriter& dataStructureWriter, const INodeGeometry2D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = INodeGeom1dIO::WriteNodeGeom1dData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());

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
