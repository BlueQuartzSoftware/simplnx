#include "ImageGeomIO.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/ImageMetadataImport.hpp"
#include <cstddef>
#include <utility>

#include "DataStructureReader.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr int32 k_ReadingGroupErrorCode = -520;
constexpr int32 k_ReadingDimensionsErrorCode = -521;
constexpr int32 k_ReadingSpacingErrorCode = -522;
constexpr int32 k_ReadingOriginErrorCode = -523;
constexpr StringLiteralType k_ReadingGroupErrorMessage = "Error opening HDF5 group while reading ImageGeom '{}'";
constexpr StringLiteralType k_ReadingDimensionsErrorMessage = "Error reading HDF5 dimensions attribute for ImageGeom '{}'";
Result<dream3d::ImageGeometryMetadata> ReadImageGeometryMetadata(const hdf5::GroupIO& group, const ImageGeom& geometry)
{
  using MetadataType = dream3d::ImageGeometryMetadata;
  const auto origin = group.readVectorAttribute<float32>(IOConstants::k_H5_ORIGIN);
  if(origin.invalid() || origin.value().size() != 3)
  {
    return MakeErrorResult<MetadataType>(k_ReadingOriginErrorCode, fmt::format("Cannot read image geometry '{}' from '{}'. Expected three origin values in the HDF5 attribute.",
                                                                               dream3d::detail::ImageGeometryPath(geometry), group.getFilePath().string()));
  }
  const auto spacing = group.readVectorAttribute<float32>(IOConstants::k_H5_SPACING);
  if(spacing.invalid() || spacing.value().size() != 3)
  {
    return MakeErrorResult<MetadataType>(k_ReadingSpacingErrorCode, fmt::format("Cannot read image geometry '{}' from '{}'. Expected three spacing values in the HDF5 attribute.",
                                                                                dream3d::detail::ImageGeometryPath(geometry), group.getFilePath().string()));
  }
  return {MetadataType{FloatVec3(origin.value()), FloatVec3(spacing.value())}};
}
} // namespace

namespace nx::core::hdf5
{
ImageGeomIO::ImageGeomIO() = default;
ImageGeomIO::~ImageGeomIO() noexcept = default;

DataObject::Type ImageGeomIO::getDataType() const
{
  return DataObject::Type::ImageGeom;
}

std::string ImageGeomIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> ImageGeomIO::readData(DataStructureReader& dataStructureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* imageGeomPtr = ImageGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  auto groupReader = parentGroup.openGroup(objectName);
  if(!groupReader.isValid())
  {
    return MakeErrorResult(k_ReadingGroupErrorCode, fmt::format(fmt::runtime(k_ReadingGroupErrorMessage.view()), objectName));
  }

  if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
  {
    imageGeomPtr->setUnits(static_cast<IGeometry::LengthUnit>(unitsAttr.value()));
  }

  auto dimensions = groupReader.readVectorAttribute<usize>(IOConstants::k_H5_DIMENSIONS);
  if(dimensions.invalid() || dimensions.value().size() != 3)
  {
    return MakeErrorResult(k_ReadingDimensionsErrorCode, fmt::format(fmt::runtime(k_ReadingDimensionsErrorMessage.view()), objectName));
  }

  imageGeomPtr->setDimensions(SizeVec3(dimensions.value()));
  auto result = dream3d::detail::ImportImageMetadata(*imageGeomPtr, groupReader.getFilePath(), ReadImageGeometryMetadata(groupReader, *imageGeomPtr), dataStructureReader.deferredImageMetadata());
  if(result.invalid())
  {
    return result;
  }
  return MergeResults(std::move(result), IGridGeometryIO::ReadGridGeometryData(dataStructureReader, *imageGeomPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore));
}

Result<> ImageGeomIO::writeData(DataStructureWriter& dataStructureWriter, const ImageGeom& geometry, GroupWriterType& parentGroupWriter, bool importable)
{
  Result<> result = IGridGeometryIO::WriteGridGeometryData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());

  AppendWriteResult(result, groupWriter.writeVectorAttribute(IOConstants::k_H5_DIMENSIONS, geometry.getDimensions().toContainer<std::vector<size_t>>()));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry dimensions for '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }
  AppendWriteResult(result, groupWriter.writeVectorAttribute(IOConstants::k_H5_ORIGIN, geometry.getOrigin().toContainer<std::vector<float32>>()));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry origin for '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }
  AppendWriteResult(result, groupWriter.writeVectorAttribute(IOConstants::k_H5_SPACING, geometry.getSpacing().toContainer<std::vector<float32>>()));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry spacing for '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  AppendWriteResult(result, groupWriter.writeScalarAttribute(IOConstants::k_H5_UNITS, nx::core::to_underlying(geometry.getUnits())));
  if(result.invalid())
  {
    auto errorResult = MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry units for '{}'", geometry.getName()));
    errorResult.warnings() = std::move(result.warnings());
    return errorResult;
  }

  return result;
}

Result<> ImageGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5
