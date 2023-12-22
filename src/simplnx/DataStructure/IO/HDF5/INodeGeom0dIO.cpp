#include "INodeGeom0dIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
INodeGeom0dIO::INodeGeom0dIO() = default;
INodeGeom0dIO::~INodeGeom0dIO() noexcept = default;

Result<> INodeGeom0dIO::ReadNodeGeom0dData(DataStructureReader& dataStructureReader, INodeGeometry0D& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                           DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = IGeometryIO::ReadGeometryData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  geometry.setVertexListId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
  geometry.setVertexDataId(ReadDataId(groupReader, IOConstants::k_VertexDataTag));

  return {};
}
Result<> INodeGeom0dIO::WriteNodeGeom0dData(DataStructureWriter& dataStructureWriter, const INodeGeometry0D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = IGeometryIO::WriteGeometryData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  nx::core::HDF5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(geometry.getName());

  DataObject::OptionalId vertexListId = geometry.getVertexListId();

  result = WriteDataId(groupWriter, vertexListId, IOConstants::k_VertexListTag);
  if(result.invalid())
  {
    return result;
  }

  if(vertexListId.has_value())
  {
    usize numVerts = geometry.getNumberOfVertices();
    auto datasetWriter = groupWriter.createDatasetWriter("_VertexIndices");
    std::vector<int64> indices(numVerts);
    for(usize i = 0; i < numVerts; i++)
    {
      indices[i] = i;
    }
    result = datasetWriter.writeSpan(nx::core::HDF5::DatasetWriter::DimsType{numVerts, 1}, nonstd::span<const int64>{indices});
    if(result.invalid())
    {
      std::string ss = "Failed to write indices to dataset";
      return MakeErrorResult(result.errors()[0].code, ss);
    }
  }

  result = WriteDataId(groupWriter, geometry.getVertexAttributeMatrixId(), IOConstants::k_VertexDataTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
} // namespace nx::core::HDF5
