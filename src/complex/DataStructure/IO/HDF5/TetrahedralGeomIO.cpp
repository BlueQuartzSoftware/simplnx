#include "TetrahedralGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
TetrahedralGeomIO::TetrahedralGeomIO() = default;
TetrahedralGeomIO::~TetrahedralGeomIO() noexcept = default;

Result<> TetrahedralGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                     const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  m_TriListId = ReadH5DataId(groupReader, H5Constants::k_TriListTag);
  m_UnsharedTriListId = ReadH5DataId(groupReader, H5Constants::k_UnsharedTriListTag);
  m_TetListId = ReadH5DataId(groupReader, H5Constants::k_TetListTag);
  m_TetsContainingVertId = ReadH5DataId(groupReader, H5Constants::k_TetsContainingVertTag);
  m_TetNeighborsId = ReadH5DataId(groupReader, H5Constants::k_TetNeighborsTag);
  m_TetCentroidsId = ReadH5DataId(groupReader, H5Constants::k_TetCentroidsTag);
  m_TetSizesId = ReadH5DataId(groupReader, H5Constants::k_TetSizesTag);

  return AbstractGeometry3D::readHdf5(dataStructureReader, groupReader, preflight);
}
Result<> TetrahedralGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
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
} // namespace complex::HDF5
