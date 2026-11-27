#include "INodeGeom1dIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "DataStructureReader.hpp"
#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::hdf5
{
INodeGeom1dIO::INodeGeom1dIO() = default;
INodeGeom1dIO::~INodeGeom1dIO() noexcept = default;

Result<> INodeGeom1dIO::ReadNodeGeom1dData(DataStructureReader& dataStructureReader, INodeGeometry1D& geometry, const GroupReaderType& parentGroup, const std::string& objectName,
                                           DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = INodeGeom0dIO::ReadNodeGeom0dData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  geometry.setEdgeListId(ReadDataId(groupReader, io_constants::k_EdgeListTag));
  geometry.setEdgeDataId(ReadDataId(groupReader, io_constants::k_EdgeDataTag));
  geometry.setElementContainingVertId(ReadDataId(groupReader, io_constants::k_ElementContainingVertTag));
  geometry.setElementNeighborsId(ReadDataId(groupReader, io_constants::k_ElementNeighborsTag));
  geometry.setElementCentroidsId(ReadDataId(groupReader, io_constants::k_ElementCentroidTag));

  return {};
}

Result<> INodeGeom1dIO::FinishImportingNodeGeom1dData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& dataStructureGroup)
{
  auto* geomPtr = dataStructure.getDataAs<INodeGeometry1D>(dataPath);
  if(geomPtr == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing INodeGeometry1D at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return INodeGeom0dIO::FinishImportingNodeGeom0dData(dataStructure, dataPath, dataStructureGroup);
}

Result<> INodeGeom1dIO::WriteNodeGeom1dData(DataStructureWriter& dataStructureWriter, const INodeGeometry1D& geometry, GroupWriterType& parentGroupWriter, bool importable)
{
  Result<> result = INodeGeom0dIO::WriteNodeGeom0dData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());
  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getEdgeListId(), io_constants::k_EdgeListTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getEdgeAttributeMatrixId(), io_constants::k_EdgeDataTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteConnectivityId(dataStructureWriter, groupWriter, geometry.getElementContainingVertId(), io_constants::k_ElementContainingVertTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteConnectivityId(dataStructureWriter, groupWriter, geometry.getElementNeighborsId(), io_constants::k_ElementNeighborsTag));
  if(result.invalid())
  {
    return result;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getElementCentroidsId(), io_constants::k_ElementCentroidTag));
  if(result.invalid())
  {
    return result;
  }

  return result;
}

Result<> INodeGeom1dIO::WriteConnectivityId(const DataStructureWriter& dataStructureWriter, GroupWriterType& groupWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag)
{
  if(objectId.has_value() && !dataStructureWriter.isOmittedConnectivity(objectId.value()))
  {
    return WriteDataId(groupWriter, objectId, tag);
  }
  return groupWriter.removeAttribute(tag);
}
} // namespace nx::core::hdf5
