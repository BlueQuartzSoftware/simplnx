#include "IGridGeometryIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::hdf5
{
IGridGeometryIO::IGridGeometryIO() = default;
IGridGeometryIO::~IGridGeometryIO() noexcept = default;

Result<> IGridGeometryIO::ReadGridGeometryData(DataStructureReader& dataStructureReader, IGridGeometry& geometry, const GroupReaderType& parentGroup, const std::string& objectName,
                                               DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  Result<> result = IGeometryIO::ReadGeometryData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);
  IGeometry::OptionalId cellDataId = ReadDataId(groupReader, IOConstants::k_CellDataTag);

  geometry.setCellData(cellDataId);

  return result;
}

Result<> IGridGeometryIO::FinishImportingGridGeometryData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& /*dataStructureGroup*/)
{
  auto* geomPtr = dataStructure.getDataAs<IGridGeometry>(dataPath);
  if(geomPtr == nullptr)
  {
    return MakeErrorResult(-50594, fmt::format("Failed to finish importing IGridGeometry at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return {};
}

Result<> IGridGeometryIO::WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const IGridGeometry& geometry, GroupWriterType& parentGroup, bool importable)
{
  auto result = IGeometryIO::WriteGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroup(geometry.getName());
  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getCellDataId(), IOConstants::k_CellDataTag));
  return result;
}
} // namespace nx::core::hdf5
