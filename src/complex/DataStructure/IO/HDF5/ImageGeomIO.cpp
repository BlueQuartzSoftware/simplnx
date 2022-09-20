#include "ImageGeomIO.hpp"

#include "DataArrayIO.hpp"
#include "complex/DataStructure/IO/HDF5/IOUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
ImageGeomIO::ImageGeomIO() = default;
ImageGeomIO::~ImageGeomIO() noexcept = default;

Result<> ImageGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  return ReadingBaseGroup(DataStructureReader, groupReader, useEmptyDataStore);
}

Result<> ImageGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  herr_t errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  SizeVec3 volDims = getDimensions();
  FloatVec3 spacing = getSpacing();
  FloatVec3 origin = getOrigin();
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

  auto dimensionAttr = groupWriter.createAttribute(H5Constants::k_H5_DIMENSIONS);
  errorCode = dimensionAttr.writeVector(dims, volDimsVector);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto originAttr = groupWriter.createAttribute(H5Constants::k_H5_ORIGIN);
  errorCode = originAttr.writeVector(dims, originVector);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto spacingAttr = groupWriter.createAttribute(H5Constants::k_H5_SPACING);
  errorCode = spacingAttr.writeVector(dims, spacingVector);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject ID
  errorCode = WriteH5DataId(groupWriter, m_VoxelSizesId, H5Constants::k_VoxelSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5
