#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace nx::core
{
class INodeGeometry0D;

namespace HDF5
{
class SIMPLNX_EXPORT INodeGeom0dIO : public IGeometryIO
{
public:
  INodeGeom0dIO();
  ~INodeGeom0dIO() noexcept override;

  INodeGeom0dIO(const INodeGeom0dIO& other) = delete;
  INodeGeom0dIO(INodeGeom0dIO&& other) = delete;
  INodeGeom0dIO& operator=(const INodeGeom0dIO& rhs) = delete;
  INodeGeom0dIO& operator=(INodeGeom0dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the INodeGeom0D from HDF5.
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
  static Result<> ReadNodeGeom0dData(DataStructureReader& dataStructureReader, INodeGeometry0D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeom0D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom0dData(DataStructureWriter& dataStructureWriter, const INodeGeometry0D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace nx::core
