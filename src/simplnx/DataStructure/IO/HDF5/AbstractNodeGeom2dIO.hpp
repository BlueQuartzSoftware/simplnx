#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractNodeGeom1dIO.hpp"

namespace nx::core
{
class AbstractNodeGeometry2D;

namespace HDF5
{
class SIMPLNX_EXPORT AbstractNodeGeom2dIO : public AbstractNodeGeom1dIO
{
public:
  AbstractNodeGeom2dIO();
  ~AbstractNodeGeom2dIO() noexcept override;

  AbstractNodeGeom2dIO(const AbstractNodeGeom2dIO& other) = delete;
  AbstractNodeGeom2dIO(AbstractNodeGeom2dIO&& other) = delete;
  AbstractNodeGeom2dIO& operator=(const AbstractNodeGeom2dIO& rhs) = delete;
  AbstractNodeGeom2dIO& operator=(AbstractNodeGeom2dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the AbstractNodeGeometry2D data from HDF5.
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
  static Result<> ReadNodeGeom2dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry2D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the AbstractNodeGeometry2D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom2dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry2D& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingNodeGeom2dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
