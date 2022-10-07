#include "IGridGeometryIO.hpp"

#include "DataStructureWriter.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"

namespace complex::HDF5
{
IGridGeometryIO::IGridGeometryIO() = default;
IGridGeometryIO::~IGridGeometryIO() noexcept = default;

Result<> IGridGeometryIO::ReadGridGeometryData(DataStructureReader& dataStructureReader, IGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& objectName,
                                               DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
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
Result<> IGridGeometryIO::WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const IGridGeometry& geometry, group_writer_type& parentGroup, bool importable)
{
  Result<> result = BaseGroupIO::WriteBaseGroupData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  H5::GroupWriter groupWriter = parentGroup.createGroupWriter(geometry.getName());
  result = WriteDataId(groupWriter, geometry.getElementSizesId(), IOConstants::k_ElementSizesTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
} // namespace complex::HDF5
