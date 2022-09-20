#include "TriangleGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
TriangleGeomIO::TriangleGeomIO() = default;
TriangleGeomIO::~TriangleGeomIO() noexcept = default;

Result<> TriangleGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  m_TriListId = ReadH5DataId(groupReader, H5Constants::k_TriangleListTag);
  m_TrianglesContainingVertId = ReadH5DataId(groupReader, H5Constants::k_TrianglesContainingVertTag);
  m_TriangleNeighborsId = ReadH5DataId(groupReader, H5Constants::k_TriangleNeighborsTag);
  m_TriangleCentroidsId = ReadH5DataId(groupReader, H5Constants::k_TriangleCentroidsTag);
  m_TriangleSizesId = ReadH5DataId(groupReader, H5Constants::k_TriangleSizesTag);

  return AbstractGeometry2D::readHdf5(dataStructureReader, groupReader, preflight);
}
Result<> TriangleGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto errorCode = AbstractGeometry2D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_TriListId, H5Constants::k_TriangleListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TrianglesContainingVertId, H5Constants::k_TrianglesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleNeighborsId, H5Constants::k_TriangleNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleCentroidsId, H5Constants::k_TriangleCentroidsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleSizesId, H5Constants::k_TriangleSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5
