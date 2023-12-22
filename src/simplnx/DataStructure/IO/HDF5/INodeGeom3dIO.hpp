#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/INodeGeom2dIO.hpp"

namespace nx::core
{
class INodeGeometry3D;

namespace HDF5
{
class SIMPLNX_EXPORT INodeGeom3dIO : public INodeGeom2dIO
{
public:
  INodeGeom3dIO();
  virtual ~INodeGeom3dIO() noexcept;

  INodeGeom3dIO(const INodeGeom3dIO& other) = delete;
  INodeGeom3dIO(INodeGeom3dIO&& other) = delete;
  INodeGeom3dIO& operator=(const INodeGeom3dIO& rhs) = delete;
  INodeGeom3dIO& operator=(INodeGeom3dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the INodeGeometry3D data from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param geometry
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  static Result<> ReadNodeGeom3dData(DataStructureReader& dataStructureReader, INodeGeometry3D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeometry3D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom3dData(DataStructureWriter& dataStructureWriter, const INodeGeometry3D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace nx::core
