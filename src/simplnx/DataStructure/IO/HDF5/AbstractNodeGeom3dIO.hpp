#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractNodeGeom2dIO.hpp"

namespace nx::core
{
class AbstractNodeGeometry3D;

namespace HDF5
{
class SIMPLNX_EXPORT AbstractNodeGeom3dIO : public AbstractNodeGeom2dIO
{
public:
  AbstractNodeGeom3dIO();
  ~AbstractNodeGeom3dIO() noexcept override;

  AbstractNodeGeom3dIO(const AbstractNodeGeom3dIO& other) = delete;
  AbstractNodeGeom3dIO(AbstractNodeGeom3dIO&& other) = delete;
  AbstractNodeGeom3dIO& operator=(const AbstractNodeGeom3dIO& rhs) = delete;
  AbstractNodeGeom3dIO& operator=(AbstractNodeGeom3dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the AbstractNodeGeometry3D data from HDF5.
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
  static Result<> ReadNodeGeom3dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry3D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the AbstractNodeGeometry3D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom3dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry3D& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingNodeGeom3dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
