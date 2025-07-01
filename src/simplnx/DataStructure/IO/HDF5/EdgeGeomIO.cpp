#include "EdgeGeomIO.hpp"
#include "DataStructureReader.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
EdgeGeomIO::EdgeGeomIO() = default;
EdgeGeomIO::~EdgeGeomIO() noexcept = default;

DataObject::Type EdgeGeomIO::getDataType() const
{
  return DataObject::Type::EdgeGeom;
}

std::string EdgeGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> EdgeGeomIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                              const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = EdgeGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  Result<> result = INodeGeom1dIO::ReadNodeGeom1dData(structureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  geometry->setVertexListId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
  geometry->setEdgeListId(ReadDataId(groupReader, IOConstants::k_EdgeListTag));
  geometry->setElementContainingVertId(ReadDataId(groupReader, IOConstants::k_ElementContainingVertTag));
  geometry->setElementNeighborsId(ReadDataId(groupReader, IOConstants::k_ElementNeighborsTag));
  geometry->setElementCentroidsId(ReadDataId(groupReader, IOConstants::k_ElementCentroidTag));
  geometry->setElementSizesId(ReadDataId(groupReader, IOConstants::k_ElementSizesTag));

  if(useEmptyDataStore)
  {
    // Add required data for preflight operations.
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_EdgeListTag));
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_ElementContainingVertTag));
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_ElementNeighborsTag));
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_ElementCentroidTag));
    structureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_ElementSizesTag));
  }

  return {};
}

Result<> EdgeGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  auto* geometry = dataStructure.getDataAs<EdgeGeom>(dataPath);
  if(geometry == nullptr)
  {
    return MakeErrorResult(-25070, fmt::format("Failed to finish importing geometry at path '{}'. Geometry does not exist or is of wrong type.", dataPath.toString()));
  }

  auto groupReader = dataStructureGroup.openGroup(dataPath.toString());
  geometry->setEdgeListId(ReadDataId(groupReader, IOConstants::k_EdgeListTag));
  geometry->setEdgeDataId(ReadDataId(groupReader, IOConstants::k_EdgeDataTag));
  geometry->setElementContainingVertId(ReadDataId(groupReader, IOConstants::k_ElementContainingVertTag));
  geometry->setElementNeighborsId(ReadDataId(groupReader, IOConstants::k_ElementNeighborsTag));
  geometry->setElementCentroidsId(ReadDataId(groupReader, IOConstants::k_ElementCentroidTag));

  return {};
}

Result<> EdgeGeomIO::writeData(DataStructureWriter& dataStructureWriter, const EdgeGeom& geometry, group_writer_type& parentGroupWriter, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());
  INodeGeom1dIO::WriteNodeGeom1dData(dataStructureWriter, geometry, parentGroupWriter, importable);

  Result<> result = WriteDataId(groupWriter, geometry.getVertexListId(), IOConstants::k_VertexListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getEdgeListId(), IOConstants::k_EdgeListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getElementContainingVertId(), IOConstants::k_ElementContainingVertTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getElementNeighborsId(), IOConstants::k_ElementNeighborsTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getElementCentroidsId(), IOConstants::k_ElementCentroidTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getElementSizesId(), IOConstants::k_ElementSizesTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> EdgeGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
