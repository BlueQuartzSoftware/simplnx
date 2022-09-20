#include "RectGridGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
RectGridGeomIO::RectGridGeomIO() = default;
RectGridGeomIO::~RectGridGeomIO() noexcept = default;

Result<> RectGridGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  // Read Dimensions
  auto volumeAttribute = groupReader.getAttribute("Dimensions");
  if(!volumeAttribute.isValid())
  {
    return -1;
  }
  std::vector<size_t> volumeDimensions = volumeAttribute.readAsVector<size_t>();
  setDimensions(volumeDimensions);

  // Read DataObject IDs
  m_xBoundsId = ReadH5DataId(groupReader, H5Constants::k_XBoundsTag);
  m_yBoundsId = ReadH5DataId(groupReader, H5Constants::k_YBoundsTag);
  m_zBoundsId = ReadH5DataId(groupReader, H5Constants::k_ZBoundsTag);
  m_VoxelSizesId = ReadH5DataId(groupReader, H5Constants::k_VoxelSizesTag);

  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}

Result<> RectGridGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write dimensions
  H5::AttributeWriter::DimsVector dims = {3};
  std::vector<size_t> dimsVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    dimsVector[i] = m_Dimensions[i];
  }

  auto dimensionAttr = groupWriter.createAttribute(H5Constants::k_DimensionsTag);
  errorCode = dimensionAttr.writeVector(dims, dimsVector);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_xBoundsId, H5Constants::k_XBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_yBoundsId, H5Constants::k_YBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_zBoundsId, H5Constants::k_ZBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_VoxelSizesId, H5Constants::k_VoxelSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5
