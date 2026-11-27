#include "EdgeGeomIO.hpp"
#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::hdf5
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

Result<> EdgeGeomIO::readData(DataStructureReader& structureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                              const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometryPtr = EdgeGeom::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  Result<> result = INodeGeom1dIO::ReadNodeGeom1dData(structureReader, *geometryPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  geometryPtr->setVertexListId(ReadDataId(groupReader, IOConstants::k_VertexListTag));
  geometryPtr->setEdgeListId(ReadDataId(groupReader, IOConstants::k_EdgeListTag));
  geometryPtr->setElementContainingVertId(ReadDataId(groupReader, IOConstants::k_ElementContainingVertTag));
  geometryPtr->setElementNeighborsId(ReadDataId(groupReader, IOConstants::k_ElementNeighborsTag));
  geometryPtr->setElementCentroidsId(ReadDataId(groupReader, IOConstants::k_ElementCentroidTag));
  geometryPtr->setElementSizesId(ReadDataId(groupReader, IOConstants::k_ElementSizesTag));

  return {};
}

Result<> EdgeGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& /*dataStructureGroup*/) const
{
  auto* geometryPtr = dataStructure.getDataAs<EdgeGeom>(dataPath);
  if(geometryPtr == nullptr)
  {
    return MakeErrorResult(-25070, fmt::format("Failed to finish importing geometry at path '{}'. Geometry does not exist or is of wrong type.", dataPath.toString()));
  }

  return {};
}

Result<> EdgeGeomIO::writeData(DataStructureWriter& dataStructureWriter, const EdgeGeom& geometry, GroupWriterType& parentGroupWriter, bool importable)
{
  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());
  auto result = INodeGeom1dIO::WriteNodeGeom1dData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getVertexListId(), IOConstants::k_VertexListTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getEdgeListId(), IOConstants::k_EdgeListTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteConnectivityId(dataStructureWriter, groupWriter, geometry.getElementContainingVertId(), IOConstants::k_ElementContainingVertTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteConnectivityId(dataStructureWriter, groupWriter, geometry.getElementNeighborsId(), IOConstants::k_ElementNeighborsTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getElementCentroidsId(), IOConstants::k_ElementCentroidTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getElementSizesId(), IOConstants::k_ElementSizesTag));
  if(result.invalid())
  {
    return result;
  }

  return result;
}

Result<> EdgeGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5
