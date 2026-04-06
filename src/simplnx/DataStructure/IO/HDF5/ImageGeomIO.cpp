#include "ImageGeomIO.hpp"

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
constexpr StringLiteral k_ReadingSpacingError_Message = "Error reading HDF5 spacing attribute for ImageGeom '{}'";
constexpr StringLiteral k_ReadingOriginError_Message = "Error reading HDF5 origin attribute for ImageGeom '{}'";
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

  std::vector<usize> volDimsVector(3);
  std::vector<float32> originVector(3);
  std::vector<float32> spacingVector(3);
  {
    auto groupReader = parentGroup.openGroup(objectName);
    if(!groupReader.isValid())
    {
      return MakeErrorResult(k_ReadingGroupError_Code, fmt::format(fmt::runtime(k_ReadingGroupError_Message.view()), objectName));
    }

    if(const auto unitsAttr = groupReader.readScalarAttribute<uint32>(IOConstants::k_H5_UNITS); unitsAttr.valid())
    {
      auto value = unitsAttr.value();
      imageGeom->setUnits(static_cast<IGeometry::LengthUnit>(value));
    }

    auto volDimsVectorResult = groupReader.readVectorAttribute<usize>(IOConstants::k_H5_DIMENSIONS);
    if(volDimsVectorResult.invalid())
    {
      return MakeErrorResult(k_ReadingDimensionsError_Code, fmt::format(fmt::runtime(k_ReadingDimensionsError_Message.view()), objectName));
    }
    volDimsVector = std::move(volDimsVectorResult.value());

    auto originVectorResult = groupReader.readVectorAttribute<float32>(IOConstants::k_H5_ORIGIN);
    if(originVectorResult.invalid())
    {
      return MakeErrorResult(k_ReadingOriginError_Code, fmt::format(fmt::runtime(k_ReadingOriginError_Message.view()), objectName));
    }
    originVector = std::move(originVectorResult.value());

    auto spacingVectorResult = groupReader.readVectorAttribute<float32>(IOConstants::k_H5_SPACING);
    if(spacingVectorResult.invalid())
    {
      return MakeErrorResult(k_ReadingSpacingError_Code, fmt::format(fmt::runtime(k_ReadingSpacingError_Message.view()), objectName));
    }
    spacingVector = std::move(spacingVectorResult.value());
  }

  SizeVec3 volDims;
  FloatVec3 spacing;
  FloatVec3 origin;
  for(usize i = 0; i < 3; i++)
  {
    volDims[i] = volDimsVector[i];
    spacing[i] = spacingVector[i];
    origin[i] = originVector[i];
  }

  imageGeom->setDimensions(volDims);
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);

  return IGridGeometryIO::ReadGridGeometryData(dataStructureReader, *imageGeom, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> ImageGeomIO::writeData(DataStructureWriter& dataStructureWriter, const ImageGeom& geometry, group_writer_type& parentGroupWriter, bool importable) const
{
  Result<> result = IGridGeometryIO::WriteGridGeometryData(dataStructureWriter, geometry, parentGroupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  auto groupWriter = parentGroupWriter.createGroup(geometry.getName());

  result = groupWriter.writeVectorAttribute(IOConstants::k_H5_DIMENSIONS, geometry.getDimensions().toContainer<std::vector<size_t>>());
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry dimensions for '{}'", geometry.getName()));
  }
  result = groupWriter.writeVectorAttribute(IOConstants::k_H5_ORIGIN, geometry.getOrigin().toContainer<std::vector<float32>>());
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry origin for '{}'", geometry.getName()));
  }
  result = groupWriter.writeVectorAttribute(IOConstants::k_H5_SPACING, geometry.getSpacing().toContainer<std::vector<float32>>());
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry spacing for '{}'", geometry.getName()));
  }

  result = groupWriter.writeScalarAttribute(IOConstants::k_H5_UNITS, nx::core::to_underlying(geometry.getUnits()));
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, fmt::format("Failed to write geometry units for '{}'", geometry.getName()));
  }

  return {};
}

Result<> ImageGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::HDF5
