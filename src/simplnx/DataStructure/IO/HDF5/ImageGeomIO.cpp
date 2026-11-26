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
constexpr int32 k_ReadingGroupError_Code = -520;
constexpr int32 k_ReadingDimensionsError_Code = -521;
constexpr int32 k_ReadingSpacingError_Code = -522;
constexpr int32 k_ReadingOriginError_Code = -523;
constexpr StringLiteral k_ReadingGroupError_Message = "Error opening HDF5 group while reading ImageGeom '{}'";
constexpr StringLiteral k_ReadingDimensionsError_Message = "Error reading HDF5 dimensions attribute for ImageGeom '{}'";
Result<DREAM3D::ImageGeometryMetadata> ReadImageGeometryMetadata(const HDF5::GroupIO& group, const ImageGeom& geometry)
{
  using Metadata = DREAM3D::ImageGeometryMetadata;
  const auto origin = group.readVectorAttribute<float32>(IOConstants::k_H5_ORIGIN);
  if(origin.invalid() || origin.value().size() != 3)
  {
    return MakeErrorResult<Metadata>(k_ReadingOriginError_Code, fmt::format("Cannot read image geometry '{}' from '{}'. Expected three origin values in the HDF5 attribute.",
                                                                            DREAM3D::detail::ImageGeometryPath(geometry), group.getFilePath().string()));
  }
  const auto spacing = group.readVectorAttribute<float32>(IOConstants::k_H5_SPACING);
  if(spacing.invalid() || spacing.value().size() != 3)
  {
    return MakeErrorResult<Metadata>(k_ReadingSpacingError_Code, fmt::format("Cannot read image geometry '{}' from '{}'. Expected three spacing values in the HDF5 attribute.",
                                                                             DREAM3D::detail::ImageGeometryPath(geometry), group.getFilePath().string()));
  }
  return {Metadata{FloatVec3(origin.value()), FloatVec3(spacing.value())}};
}
} // namespace

namespace nx::core::HDF5
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

Result<> ImageGeomIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* imageGeom = ImageGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  auto groupReader = parentGroup.openGroup(objectName);
  if(!groupReader.isValid())
  {
    return MakeErrorResult(k_ReadingGroupError_Code, fmt::format(fmt::runtime(k_ReadingGroupError_Message.view()), objectName));
  }

  if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
  {
    imageGeom->setUnits(static_cast<IGeometry::LengthUnit>(unitsAttr.value()));
  }

  auto dimensions = groupReader.readVectorAttribute<usize>(IOConstants::k_H5_DIMENSIONS);
  if(dimensions.invalid() || dimensions.value().size() != 3)
  {
    return MakeErrorResult(k_ReadingDimensionsError_Code, fmt::format(fmt::runtime(k_ReadingDimensionsError_Message.view()), objectName));
  }

  imageGeom->setDimensions(SizeVec3(dimensions.value()));
  auto result = DREAM3D::detail::ImportImageMetadata(*imageGeom, groupReader.getFilePath(), ReadImageGeometryMetadata(groupReader, *imageGeom), dataStructureReader.deferredImageMetadata());
  if(result.invalid())
  {
    return result;
  }
  return MergeResults(std::move(result), IGridGeometryIO::ReadGridGeometryData(dataStructureReader, *imageGeom, parentGroup, objectName, importId, parentId, useEmptyDataStore));
}

Result<> ImageGeomIO::writeData(DataStructureWriter& dataStructureWriter, const ImageGeom& geometry, group_writer_type& parentGroupWriter, bool importable) 
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

Result<> ImageGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
