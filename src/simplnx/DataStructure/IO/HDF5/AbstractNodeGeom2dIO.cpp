#include "AbstractNodeGeom2dIO.hpp"

#include "DataStructureReader.hpp"
#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
AbstractNodeGeom2dIO::AbstractNodeGeom2dIO() = default;
AbstractNodeGeom2dIO::~AbstractNodeGeom2dIO() noexcept = default;

Result<> AbstractNodeGeom2dIO::ReadNodeGeom2dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry2D& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                                  AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = AbstractNodeGeom1dIO::ReadNodeGeom1dData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  geometry.setFaceListId(ReadDataId(groupReader, IOConstants::k_FaceListTag));
  geometry.setFaceDataId(ReadDataId(groupReader, IOConstants::k_FaceDataTag));
  geometry.setUnsharedEdgesId(ReadDataId(groupReader, IOConstants::k_UnsharedEdgeListTag));

  // Required data
  if(useEmptyDataStore)
  {
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_FaceListTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_FaceDataTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_UnsharedEdgeListTag));
  }

  return {};
}

Result<> AbstractNodeGeom2dIO::FinishImportingNodeGeom2dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<AbstractNodeGeometry2D>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50591, fmt::format("Failed to finish importing AbstractNodeGeometry2D at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return AbstractNodeGeom1dIO::FinishImportingNodeGeom1dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> AbstractNodeGeom2dIO::WriteNodeGeom2dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry2D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = AbstractNodeGeom1dIO::WriteNodeGeom1dData(dataStructureWriter, geometry, parentGroupWriter, importable);
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
