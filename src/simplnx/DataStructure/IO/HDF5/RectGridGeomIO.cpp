#include "RectGridGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

namespace nx::core::HDF5
{
RectGridGeomIO::RectGridGeomIO() = default;
RectGridGeomIO::~RectGridGeomIO() noexcept = default;

AbstractDataObject::Type RectGridGeomIO::getDataType() const
{
  return IDataObject::Type::RectGridGeom;
}

std::string RectGridGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> RectGridGeomIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, AbstractDataObject::IdType importId,
                                  const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometry = RectGridGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  Result<> result = AbstractGridGeometryIO::ReadGridGeometryData(dataStructureReader, *geometry, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
  {
    auto value = unitsAttr.value();
    geometry->setUnits(static_cast<AbstractGeometry::LengthUnit>(value));
  }

  // Read Dimensions
  auto volumeDimensionsResult = groupReader.readVectorAttribute<usize>("Dimensions");
  if(volumeDimensionsResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(volumeDimensionsResult));
  }
  const std::vector<size_t> volumeDimensions = std::move(volumeDimensionsResult.value());

  geometry->setDimensions(volumeDimensions);

  // Read AbstractDataObject IDs
  geometry->setXBoundsId(ReadDataId(groupReader, IOConstants::k_XBoundsTag));
  geometry->setYBoundsId(ReadDataId(groupReader, IOConstants::k_YBoundsTag));
  geometry->setZBoundsId(ReadDataId(groupReader, IOConstants::k_ZBoundsTag));

  // Required data
  if(useEmptyDataStore)
  {
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_XBoundsTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_YBoundsTag));
    dataStructureReader.addRequiredId(ReadDataId(groupReader, IOConstants::k_ZBoundsTag));
  }

  return {};
}

Result<> RectGridGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  auto* geom = dataStructure.getDataAs<RectGridGeom>(dataPath);
  if(geom == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing RectGridGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  {
    auto groupReader = dataStructureGroup.openGroup(dataPath.toString());

    if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
    {
      auto value = unitsAttr.value();
      geom->setUnits(static_cast<AbstractGeometry::LengthUnit>(value));
    }

    // Read Dimensions
    auto volumeDimensionsResult = groupReader.readVectorAttribute<usize>("Dimensions");
    if(volumeDimensionsResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(volumeDimensionsResult));
    }
    const std::vector<size_t> volumeDimensions = std::move(volumeDimensionsResult.value());

    geom->setDimensions(volumeDimensions);
  }

  return AbstractGridGeometryIO::FinishImportingGridGeometryData(dataStructure, dataPath, dataStructureGroup);
}

Result<> RectGridGeomIO::writeData(DataStructureWriter& dataStructureWriter, const RectGridGeom& geometry, group_writer_type& parentGroup, bool importable) const
{
  Result<> result = AbstractGridGeometryIO::WriteGridGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroup(geometry.getName());

  // Write dimensions
  auto dimensions = geometry.getDimensions();
  std::vector<size_t> dimsVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    dimsVector[i] = dimensions[i];
  }

  result = groupWriter.writeVectorAttribute(IOConstants::k_DimensionsTag, dimsVector);
  if(result.invalid())
  {
    return result;
  }
  // Write AbstractDataObject IDs
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

  result = groupWriter.writeScalarAttribute(IOConstants::k_H5_UNITS, nx::core::to_underlying(geometry.getUnits()));
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, "Failed to write geometry units");
  }

  return {};
}

Result<> RectGridGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
