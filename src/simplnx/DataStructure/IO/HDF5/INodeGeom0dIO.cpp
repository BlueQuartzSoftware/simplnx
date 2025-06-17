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

  if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
  {
    auto value = unitsAttr.value();
    geometry.setUnits(static_cast<IGeometry::LengthUnit>(value));
  }

  geometry.setVertexListId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
  geometry.setVertexDataId(ReadDataId(groupReader, IOConstants::k_VertexDataTag));

  return {};
}

Result<> INodeGeom0dIO::FinishImportingNodeGeom0dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<INodeGeometry0D>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing INodeGeometry0D at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  {
    auto groupReader = dataStructureGroup.openGroup(dataPath.toString());
    if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
    {
      auto value = unitsAttr.value();
      geom->setUnits(static_cast<IGeometry::LengthUnit>(value));
    }

    geom->setVertexListId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
    geom->setVertexDataId(ReadDataId(groupReader, IOConstants::k_VertexDataTag));
  }

  return IGeometryIO::FinishImportingGeomData(dataStructure, dataPath, dataStructureGroup);
}

Result<> INodeGeom0dIO::WriteNodeGeom0dData(DataStructureWriter& dataStructureWriter, const INodeGeometry0D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = IGeometryIO::WriteGeometryData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());

  DataObject::OptionalId vertexListId = geometry.getVertexListId();

  result = WriteDataId(groupWriter, vertexListId, IOConstants::k_VertexListTag);
  if(result.invalid())
  {
    return result;
  }

  if(vertexListId.has_value())
  {
    usize numVerts = geometry.getNumberOfVertices();
    auto datasetWriter = groupWriter.createDataset("_VertexIndices");

    std::vector<int64> indices(numVerts);
    for(usize i = 0; i < numVerts; i++)
    {
      indices[i] = i;
    }
    result = datasetWriter.writeSpan(nx::core::HDF5::DatasetIO::DimsType{numVerts, 1}, nonstd::span<const int64>{indices});
    if(result.invalid())
    {
      std::string ss = "Failed to write indices to dataset";
      return MakeErrorResult(result.errors()[0].code, ss);
    }
  }

  result = groupWriter.writeScalarAttribute(IOConstants::k_H5_UNITS, nx::core::to_underlying(geometry.getUnits()));
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, "Failed to write geometry units");
  }

  result = WriteDataId(groupWriter, geometry.getVertexAttributeMatrixId(), IOConstants::k_VertexDataTag);
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, "Failed to write vertex attribute matrix");
  }

  return {};
}
} // namespace nx::core::HDF5
