#include "IGridGeometryIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
IGridGeometryIO::IGridGeometryIO() = default;
IGridGeometryIO::~IGridGeometryIO() noexcept = default;

Result<> IGridGeometryIO::ReadGridGeometryData(DataStructureReader& dataStructureReader, IGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                               DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  auto groupReader = parentGroup.openGroup(objectName);
  Result<> result = IGeometryIO::ReadGeometryData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  IGeometry::OptionalId cellDataId = ReadDataId(groupReader, IOConstants::k_CellDataTag);

  geometry.setCellData(cellDataId);

  return {};
}
Result<> IGridGeometryIO::WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const IGridGeometry& geometry, group_writer_type& parentGroup, bool importable)
{
  auto result = IGeometryIO::WriteGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  nx::core::HDF5::GroupWriter groupWriter = parentGroup.createGroupWriter(geometry.getName());
  Result<> writeResult = WriteDataId(groupWriter, geometry.getCellDataId(), IOConstants::k_CellDataTag);
  return writeResult;
}
} // namespace nx::core::HDF5
