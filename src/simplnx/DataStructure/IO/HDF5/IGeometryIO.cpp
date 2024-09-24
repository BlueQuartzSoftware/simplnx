#include "IGeometryIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
IGeometryIO::IGeometryIO() = default;
IGeometryIO::~IGeometryIO() noexcept = default;

Result<> IGeometryIO::ReadGeometryData(DataStructureReader& dataStructureReader, IGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                       const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  auto groupReaderResult = parentGroup.openGroup(objectName);
  if(groupReaderResult.invalid())
  {
    return ConvertResult(std::move(groupReaderResult));
  }
  auto groupReader = std::move(groupReaderResult.value());

  Result<> result = BaseGroupIO::ReadBaseGroupData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  geometry.setElementSizesId(ReadDataId(groupReader, IOConstants::k_ElementSizesTag));

  return {};
}
Result<> IGeometryIO::WriteGeometryData(DataStructureWriter& dataStructureWriter, const IGeometry& geometry, group_writer_type& parentGroup, bool importable)
{
  auto groupWriterResult = parentGroup.createGroup(geometry.getName());
  if(groupWriterResult.invalid())
  {
    return ConvertResult(std::move(groupWriterResult));
  }
  auto groupWriter = std::move(groupWriterResult.value());

  Result<> result = WriteDataId(groupWriter, geometry.getElementSizesId(), IOConstants::k_ElementSizesTag);
  if(result.invalid())
  {
    return result;
  }

  return BaseGroupIO::WriteBaseGroupData(dataStructureWriter, geometry, parentGroup, importable);
}

} // namespace nx::core::HDF5
