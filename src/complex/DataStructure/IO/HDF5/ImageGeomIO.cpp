#include "ImageGeomIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IO/Generic/IOConstants.hpp"
#include "complex/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "complex/DataStructure/IO/HDF5/IOUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include "fmt/format.h"

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
  result = WriteObjectAttributes(dataStructureWriter, geometry, groupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  SizeVec3 volDims = geometry.getDimensions();
  FloatVec3 spacing = geometry.getSpacing();
  FloatVec3 origin = geometry.getOrigin();
  H5::AttributeWriter::DimsVector dims = {3};
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
    std::string ss = fmt::format("Failed to write volume dimensions");
    return MakeErrorResult(errorCode, ss);
  }

  auto originAttr = groupWriter.createAttribute(IOConstants::k_H5_ORIGIN);
  errorCode = originAttr.writeVector(dims, originVector);
  if(errorCode < 0)
  {
    std::string ss = fmt::format("Failed to write volume origin");
    return MakeErrorResult(errorCode, ss);
  }

  auto spacingAttr = groupWriter.createAttribute(IOConstants::k_H5_SPACING);
  errorCode = spacingAttr.writeVector(dims, spacingVector);
  if(errorCode < 0)
  {
    std::string ss = fmt::format("Failed to write volume spacing");
    return MakeErrorResult(errorCode, ss);
  }

  return {};
}

Result<> ImageGeomIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  auto* targetData = dynamic_cast<const data_type*>(dataObject);
  if(targetData == nullptr)
  {
    std::string ss = "Provided DataObject could not be cast to the target type";
    return MakeErrorResult(-800, ss);
  }

  return writeData(dataStructureWriter, *targetData, parentWriter, true);
}
} // namespace complex::HDF5
