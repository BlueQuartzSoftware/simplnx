#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/INodeGeom1dIO.hpp"

namespace nx::core
{
class INodeGeometry2D;

namespace HDF5
{
class SIMPLNX_EXPORT INodeGeom2dIO : public INodeGeom1dIO
{
public:
  INodeGeom2dIO();
  ~INodeGeom2dIO() noexcept override;

  INodeGeom2dIO(const INodeGeom2dIO& other) = delete;
  INodeGeom2dIO(INodeGeom2dIO&& other) = delete;
  INodeGeom2dIO& operator=(const INodeGeom2dIO& rhs) = delete;
  INodeGeom2dIO& operator=(INodeGeom2dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the INodeGeometry2D data from HDF5.
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
  static Result<> ReadNodeGeom2dData(DataStructureReader& dataStructureReader, INodeGeometry2D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeometry2D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom2dData(DataStructureWriter& dataStructureWriter, const INodeGeometry2D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace nx::core
