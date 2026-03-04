#include "AbstractGeometryIO.hpp"

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

namespace nx::core::HDF5
{
AbstractGeometryIO::AbstractGeometryIO() = default;
AbstractGeometryIO::~AbstractGeometryIO() noexcept = default;

Result<> AbstractGeometryIO::ReadGeometryData(DataStructureReader& dataStructureReader, AbstractGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                              AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore)
{
  auto groupReader = parentGroup.openGroup(objectName);

  Result<> result = BaseGroupIO::ReadBaseGroupData(dataStructureReader, geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  geometry.setElementSizesId(ReadDataId(groupReader, IOConstants::k_ElementSizesTag));

  return {};
}

Result<> AbstractGeometryIO::FinishImportingGeomData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup)
{
  auto* geom = dataStructure.getDataAs<AbstractGeometry>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing AbstractGeometry at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  return {};
}

Result<> AbstractGeometryIO::WriteGeometryData(DataStructureWriter& dataStructureWriter, const AbstractGeometry& geometry, group_writer_type& parentGroup, bool importable)
{
  auto groupWriter = parentGroup.createGroup(geometry.getName());

  Result<> result = WriteDataId(groupWriter, geometry.getElementSizesId(), IOConstants::k_ElementSizesTag);
  if(result.invalid())
  {
    return result;
  }

  return BaseGroupIO::WriteBaseGroupData(dataStructureWriter, geometry, parentGroup, importable);
}

} // namespace nx::core::HDF5
