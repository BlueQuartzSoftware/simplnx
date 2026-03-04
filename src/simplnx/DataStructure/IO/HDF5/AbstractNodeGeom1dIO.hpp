#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractNodeGeom0dIO.hpp"

namespace nx::core
{
class AbstractNodeGeometry1D;

namespace HDF5
{
class SIMPLNX_EXPORT AbstractNodeGeom1dIO : public AbstractNodeGeom0dIO
{
public:
  AbstractNodeGeom1dIO();
  ~AbstractNodeGeom1dIO() noexcept override;

  AbstractNodeGeom1dIO(const AbstractNodeGeom1dIO& other) = delete;
  AbstractNodeGeom1dIO(AbstractNodeGeom1dIO&& other) = delete;
  AbstractNodeGeom1dIO& operator=(const AbstractNodeGeom1dIO& rhs) = delete;
  AbstractNodeGeom1dIO& operator=(AbstractNodeGeom1dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the AbstractNodeGeometry1D data from HDF5.
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
  static Result<> ReadNodeGeom1dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry1D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the AbstractNodeGeometry1D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom1dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry1D& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingNodeGeom1dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
