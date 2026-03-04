#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class AbstractGeometry;

namespace HDF5
{
/**
 * @brief Base class for simplnx geometry IO using HDF5
 */
class SIMPLNX_EXPORT AbstractGeometryIO : public BaseGroupIO
{
public:
  AbstractGeometryIO();
  virtual ~AbstractGeometryIO() noexcept;

  AbstractGeometryIO(const AbstractGeometryIO& other) = delete;
  AbstractGeometryIO(AbstractGeometryIO&& other) = delete;
  AbstractGeometryIO& operator=(const AbstractGeometryIO& rhs) = delete;
  AbstractGeometryIO& operator=(AbstractGeometryIO&& rhs) = delete;

protected:
  /**
   * @brief Imports the geometry's data from the corresponding HDF5 object.
   * Returns a result with any errors or warnings that were encountered during the import process.
   * @param dataStructureReader
   * @param geometry
   * @param parentGroup
   * @param geometryName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore
   * @return Result<>
   */
  static Result<> ReadGeometryData(DataStructureReader& dataStructureReader, AbstractGeometry& geometry, const group_reader_type& parentGroup, const std::string& geometryName,
                                   AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Writes the generic geometry data to HDF5.
   * Returns a Result with any errors or warnings that were encountered during the export process.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteGeometryData(DataStructureWriter& dataStructureWriter, const AbstractGeometry& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingGeomData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
