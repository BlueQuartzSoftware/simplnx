#include "RectGridGeomIO.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"
#include <utility>

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

namespace nx::core::hdf5
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

Result<> RectGridGeomIO::readData(DataStructureReader& dataStructureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* geometryPtr = RectGridGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  Result<> result = IGridGeometryIO::ReadGridGeometryData(dataStructureReader, *geometryPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore);
  if(result.invalid())
  {
    return result;
  }

  auto groupReader = parentGroup.openGroup(objectName);

  if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(io_constants::k_H5Units); unitsAttr.valid())
  {
    auto value = unitsAttr.value();
    geometryPtr->setUnits(static_cast<IGeometry::LengthUnit>(value));
  }

  // Read the geometry dimensions.
  auto volumeDimensionsResult = groupReader.readVectorAttribute<usize>("Dimensions");
  if(volumeDimensionsResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(volumeDimensionsResult));
  }
  const std::vector<size_t> volumeDimensions = std::move(volumeDimensionsResult.value());

  geometryPtr->setDimensions(volumeDimensions);

  // A bounds array may be stored outside this group. Resolve and validate its values after the full map is loaded.
  geometryPtr->setXBoundsId(ReadDataId(groupReader, io_constants::k_XBoundsTag));
  geometryPtr->setYBoundsId(ReadDataId(groupReader, io_constants::k_YBoundsTag));
  geometryPtr->setZBoundsId(ReadDataId(groupReader, io_constants::k_ZBoundsTag));

  return result;
}

Result<> RectGridGeomIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& dataStructureGroup) const
{
  auto* geomPtr = dataStructure.getDataAs<RectGridGeom>(dataPath);
  if(geomPtr == nullptr)
  {
    return MakeErrorResult(-50590, fmt::format("Failed to finish importing RectGridGeom at path '{}'. Data not found or of incorrect type.", dataPath.toString()));
  }

  {
    auto groupReader = dataStructureGroup.openGroup(dataPath.getTargetName());
    if(!groupReader.isValid())
    {
      return MakeErrorResult(-50597, fmt::format("Failed to finish importing geometry at path '{}': could not open its HDF5 group '{}' inside parent group '{}'. The file may be corrupt or was "
                                                 "written by an incompatible version of DREAM3D-NX.",
                                                 dataPath.toString(), dataPath.getTargetName(), dataStructureGroup.getObjectPath()));
    }

    if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(io_constants::k_H5Units); unitsAttr.valid())
    {
      auto value = unitsAttr.value();
      geomPtr->setUnits(static_cast<IGeometry::LengthUnit>(value));
    }

    // Read the geometry dimensions.
    auto volumeDimensionsResult = groupReader.readVectorAttribute<usize>("Dimensions");
    if(volumeDimensionsResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(volumeDimensionsResult));
    }
    const std::vector<size_t> volumeDimensions = std::move(volumeDimensionsResult.value());

    geomPtr->setDimensions(volumeDimensions);
  }

  return IGridGeometryIO::FinishImportingGridGeometryData(dataStructure, dataPath, dataStructureGroup);
}

namespace
{
Result<> ValidateGridBounds(const RectGridGeom* geometryPtr, const std::filesystem::path& filePath)
{
  if(geometryPtr == nullptr)
  {
    return {};
  }
  auto result = RectGridGeom::ValidateBounds(geometryPtr->getXBounds(), geometryPtr->getYBounds(), geometryPtr->getZBounds());
  if(result.invalid())
  {
    for(auto& error : result.errors())
    {
      error.message = fmt::format("Cannot read rectilinear grid geometry '{}' from '{}': {}", geometryPtr->getName(), filePath.string(), error.message);
    }
  }
  return result;
}
} // namespace

Result<> RectGridGeomIO::ValidateImportedBounds(const DataStructure& dataStructure, const std::filesystem::path& filePath)
{
  for(const auto id : dataStructure.getAllDataObjectIds())
  {
    if(auto result = ValidateGridBounds(dataStructure.getDataAs<RectGridGeom>(id), filePath); result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> RectGridGeomIO::ValidateImportedBounds(const DataStructure& dataStructure, const std::filesystem::path& filePath, const std::vector<DataPath>& selectedPaths)
{
  for(const auto& path : selectedPaths)
  {
    if(auto result = ValidateGridBounds(dataStructure.getDataAs<RectGridGeom>(path), filePath); result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> RectGridGeomIO::writeData(DataStructureWriter& dataStructureWriter, const RectGridGeom& geometry, GroupWriterType& parentGroup, bool importable)
{
  auto validation = RectGridGeom::ValidateBounds(geometry.getXBounds(), geometry.getYBounds(), geometry.getZBounds());
  if(validation.invalid())
  {
    for(auto& error : validation.errors())
    {
      error.message = fmt::format("Cannot write rectilinear grid geometry '{}' to '{}': {}", geometry.getName(), parentGroup.getFilePath().string(), error.message);
    }
    return validation;
  }

  Result<> result = IGridGeometryIO::WriteGridGeometryData(dataStructureWriter, geometry, parentGroup, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroup.createGroup(geometry.getName());

  // Write the geometry dimensions.
  auto dimensions = geometry.getDimensions();
  std::vector<size_t> dimsVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    dimsVector[i] = dimensions[i];
  }

  AppendWriteResult(result, groupWriter.writeVectorAttribute(io_constants::k_DimensionsTag, dimsVector));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write dimensions for RectGridGeom '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }
  // Write the bounds array identifiers.
  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getXBoundsId(), io_constants::k_XBoundsTag));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write X bounds for RectGridGeom '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getYBoundsId(), io_constants::k_YBoundsTag));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write Y bounds for RectGridGeom '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  AppendWriteResult(result, WriteDataId(groupWriter, geometry.getZBoundsId(), io_constants::k_ZBoundsTag));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write Z bounds for RectGridGeom '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  AppendWriteResult(result, groupWriter.writeScalarAttribute(io_constants::k_H5Units, nx::core::to_underlying(geometry.getUnits())));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry units for RectGridGeom '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  return result;
}

Result<> RectGridGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5
