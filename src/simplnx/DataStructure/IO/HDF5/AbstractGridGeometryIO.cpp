#include "AbstractGridGeometryIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGridGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
AbstractGridGeometryIO::AbstractGridGeometryIO() = default;
AbstractGridGeometryIO::~AbstractGridGeometryIO() noexcept = default;

Result<> AbstractGridGeometryIO::ReadGridGeometryData(DataStructureReader& dataStructureReader, AbstractGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                                      AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = AbstractGeometryIO::ReadGeometryData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  AbstractGeometry::OptionalId cellDataId = ReadDataId(groupReader, IOConstants::k_CellDataTag);

  geometry.setCellData(cellDataId);

  return {};
}

Result<> AbstractGridGeometryIO::FinishImportingGridGeometryData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<AbstractGridGeometry>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50594, fmt::format("Failed to finish importing AbstractGridGeometry at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return {};
}

Result<> AbstractGridGeometryIO::WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const AbstractGridGeometry& geometry, group_writer_type& parentGroup, bool importable)
{
  auto result = AbstractGeometryIO::WriteGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroup(geometry.getName());
  Result<> writeResult = WriteDataId(groupWriter, geometry.getCellDataId(), IOConstants::k_CellDataTag);
  return writeResult;
}
} // namespace nx::core::HDF5
