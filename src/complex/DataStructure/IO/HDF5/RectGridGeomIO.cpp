#include "RectGridGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

#include "fmt/format.h"

namespace complex::HDF5
{
RectGridGeomIO::RectGridGeomIO() = default;
RectGridGeomIO::~RectGridGeomIO() noexcept = default;

DataObject::Type RectGridGeomIO::getDataType() const
{
  return DataObject::Type::RectGridGeom;
}

std::string RectGridGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> RectGridGeomIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = RectGridGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  Result<> result = IGridGeometryIO::ReadGridGeometryData(dataStructureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  // Read Dimensions
  auto volumeAttribute = groupReader.getAttribute("Dimensions");
  if(!volumeAttribute.isValid())
  {
    std::string ss = fmt::format("Failed to access Dimensions attribute");
    return MakeErrorResult(-1, ss);
  }
  std::vector<size_t> volumeDimensions = volumeAttribute.readAsVector<size_t>();
  geometry->setDimensions(volumeDimensions);

  // Read DataObject IDs
  geometry->setXBoundsId(ReadDataId(groupReader, IOConstants::k_XBoundsTag));
  geometry->setYBoundsId(ReadDataId(groupReader, IOConstants::k_YBoundsTag));
  geometry->setZBoundsId(ReadDataId(groupReader, IOConstants::k_ZBoundsTag));

  return {};
}

Result<> RectGridGeomIO::writeData(DataStructureWriter& dataStructureWriter, const RectGridGeom& geometry, group_writer_type& parentGroup, bool importable) const
{
  Result<> result = IGridGeometryIO::WriteGridGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroupWriter(geometry.getName());

  // Write dimensions
  auto dimensions = geometry.getDimensions();
  complex::HDF5::AttributeWriter::DimsVector dims = {3};
  std::vector<size_t> dimsVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    dimsVector[i] = dimensions[i];
  }

  auto dimensionAttr = groupWriter.createAttribute(IOConstants::k_DimensionsTag);
  auto writeResult = dimensionAttr.writeVector(dims, dimsVector);
  if(writeResult.invalid())
  {
    std::string ss = "Failed to write dimensions attribute";
    return MakeErrorResult(writeResult.errors()[0].code, ss);
  }

  // Write DataObject IDs
  result = WriteDataId(groupWriter, geometry.getXBoundsId(), IOConstants::k_XBoundsTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getYBoundsId(), IOConstants::k_YBoundsTag);
  if(result.invalid())
  {
    return result;
  }

  result = WriteDataId(groupWriter, geometry.getZBoundsId(), IOConstants::k_ZBoundsTag);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> RectGridGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace complex::HDF5
