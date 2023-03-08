#include "ImageGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"
#include "complex/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "complex/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"

#include "fmt/format.h"

using namespace complex;

namespace
{
constexpr int32 k_ReadingGroupError_Code = -520;
constexpr int32 k_ReadingDimensionsError_Code = -521;
constexpr int32 k_ReadingSpacingError_Code = -522;
constexpr int32 k_ReadingOriginError_Code = -523;
constexpr StringLiteral k_ReadingGroupError_Message = "Error opening HDF5 group while reading ImageGeom";
constexpr StringLiteral k_ReadingDimensionsError_Message = "Error opening HDF5 dimensions attribute while reading ImageGeom";
constexpr StringLiteral k_ReadingSpacingError_Message = "Error opening HDF5 spacing attribute while reading ImageGeom";
constexpr StringLiteral k_ReadingOriginError_Message = "Error opening HDF5 origin attribute while reading ImageGeom";
} // namespace

namespace complex::HDF5
{
ImageGeomIO::ImageGeomIO() = default;
ImageGeomIO::~ImageGeomIO() noexcept = default;

DataObject::Type ImageGeomIO::getDataType() const
{
  return DataObject::Type::ImageGeom;
}

std::string ImageGeomIO::getTypeName() const
{
  return data_type::GetTypeName();
}

Result<> ImageGeomIO::readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* imageGeom = ImageGeom::Import(dataStructureReader.getDataStructure(), objectName, importId, parentId);

  auto groupReader = parentGroup.openGroup(objectName);
  if(!groupReader.isValid())
  {
    return MakeErrorResult(k_ReadingGroupError_Code, k_ReadingGroupError_Message);
  }

  auto dimensionAttr = groupReader.getAttribute(IOConstants::k_H5_DIMENSIONS);
  if(!dimensionAttr.isValid())
  {
    return MakeErrorResult(k_ReadingDimensionsError_Code, k_ReadingDimensionsError_Message);
  }

  auto volDimsVector = dimensionAttr.readAsVector<usize>();

  auto originAttr = groupReader.getAttribute(IOConstants::k_H5_ORIGIN);
  if(!originAttr.isValid())
  {
    return MakeErrorResult(k_ReadingOriginError_Code, k_ReadingOriginError_Message);
  }
  auto originVector = originAttr.readAsVector<float32>();

  auto spacingAttr = groupReader.getAttribute(IOConstants::k_H5_SPACING);
  if(!spacingAttr.isValid())
  {
    return MakeErrorResult(k_ReadingSpacingError_Code, k_ReadingSpacingError_Message);
  }
  auto spacingVector = spacingAttr.readAsVector<float32>();

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

  auto groupWriter = parentGroupWriter.createGroupWriter(geometry.getName());

  SizeVec3 volDims = geometry.getDimensions();
  FloatVec3 spacing = geometry.getSpacing();
  FloatVec3 origin = geometry.getOrigin();
  complex::HDF5::AttributeWriter::DimsVector dims = {3};
  std::vector<size_t> volDimsVector(3);
  std::vector<float> spacingVector(3);
  std::vector<float> originVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    volDimsVector[i] = volDims[i];
    spacingVector[i] = spacing[i];
    originVector[i] = origin[i];
  }

  auto dimensionAttr = groupWriter.createAttribute(IOConstants::k_H5_DIMENSIONS);
  auto errorCode = dimensionAttr.writeVector(dims, volDimsVector);
  if(errorCode < 0)
  {
    return MakeErrorResult(errorCode, "Failed to write volume dimensions");
  }

  auto originAttr = groupWriter.createAttribute(IOConstants::k_H5_ORIGIN);
  errorCode = originAttr.writeVector(dims, originVector);
  if(errorCode < 0)
  {
    return MakeErrorResult(errorCode, "Failed to write volume origin");
  }

  auto spacingAttr = groupWriter.createAttribute(IOConstants::k_H5_SPACING);
  errorCode = spacingAttr.writeVector(dims, spacingVector);
  if(errorCode < 0)
  {
    return MakeErrorResult(errorCode, "Failed to write volume spacing");
  }

  return {};
}

Result<> ImageGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  auto* targetData = dynamic_cast<const data_type*>(dataObject);
  if(targetData == nullptr)
  {
    return MakeErrorResult(-800, "Provided DataObject could not be cast to the target type");
  }

  return writeData(dataStructureWriter, *targetData, parentWriter, true);
}
} // namespace complex::HDF5
