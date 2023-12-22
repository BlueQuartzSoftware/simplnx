#include "INodeGeom1dIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
INodeGeom1dIO::INodeGeom1dIO() = default;
INodeGeom1dIO::~INodeGeom1dIO() noexcept = default;

Result<> INodeGeom1dIO::ReadNodeGeom1dData(DataStructureReader& dataStructureReader, INodeGeometry1D& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                           DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = INodeGeom0dIO::ReadNodeGeom0dData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  geometry.setEdgeListId(ReadDataId(groupReader, IOConstants::k_EdgeListTag));
  geometry.setEdgeDataId(ReadDataId(groupReader, IOConstants::k_EdgeDataTag));
  geometry.setElementContainingVertId(ReadDataId(groupReader, IOConstants::k_ElementContainingVertTag));
  geometry.setElementNeighborsId(ReadDataId(groupReader, IOConstants::k_ElementNeighborsTag));
  geometry.setElementCentroidsId(ReadDataId(groupReader, IOConstants::k_ElementCentroidTag));

  return {};
}
Result<> INodeGeom1dIO::WriteNodeGeom1dData(DataStructureWriter& dataStructureWriter, const INodeGeometry1D& geometry, group_writer_type& parentGroupWriter, bool importable)
{
  Result<> result = INodeGeom0dIO::WriteNodeGeom0dData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  nx::core::HDF5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(geometry.getName());
  result = WriteDataId(groupWriter, geometry.getEdgeListId(), IOConstants::k_EdgeListTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getEdgeAttributeMatrixId(), IOConstants::k_EdgeDataTag);
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

  return {};
}
} // namespace nx::core::HDF5
