#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractGeometryIO.hpp"

namespace nx::core
{
class AbstractNodeGeometry0D;

namespace HDF5
{
class SIMPLNX_EXPORT AbstractNodeGeom0dIO : public AbstractGeometryIO
{
public:
  AbstractNodeGeom0dIO();
  ~AbstractNodeGeom0dIO() noexcept override;

  AbstractNodeGeom0dIO(const AbstractNodeGeom0dIO& other) = delete;
  AbstractNodeGeom0dIO(AbstractNodeGeom0dIO&& other) = delete;
  AbstractNodeGeom0dIO& operator=(const AbstractNodeGeom0dIO& rhs) = delete;
  AbstractNodeGeom0dIO& operator=(AbstractNodeGeom0dIO&& rhs) = delete;

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
  static Result<> ReadNodeGeom0dData(DataStructureReader& dataStructureReader, AbstractNodeGeometry0D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeom0D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom0dData(DataStructureWriter& dataStructureWriter, const AbstractNodeGeometry0D& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingNodeGeom0dData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
