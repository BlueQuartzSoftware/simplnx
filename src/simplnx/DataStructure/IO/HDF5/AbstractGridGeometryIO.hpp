#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractGeometryIO.hpp"

namespace nx::core
{
class AbstractGridGeometry;

namespace HDF5
{
class SIMPLNX_EXPORT AbstractGridGeometryIO : public AbstractGeometryIO
{
public:
  AbstractGridGeometryIO();
  ~AbstractGridGeometryIO() noexcept override;

  AbstractGridGeometryIO(const AbstractGridGeometryIO& other) = delete;
  AbstractGridGeometryIO(AbstractGridGeometryIO&& other) = delete;
  AbstractGridGeometryIO& operator=(const AbstractGridGeometryIO& rhs) = delete;
  AbstractGridGeometryIO& operator=(AbstractGridGeometryIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the GridGeometry data from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param geometry
   * @param parentGroup
   * @param geometryName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  static Result<> ReadGridGeometryData(DataStructureReader& dataStructureReader, AbstractGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& geometryName,
                                       AbstractDataObject::IdType importId, const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the AbstractGridGeometry data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const AbstractGridGeometry& geometry, group_writer_type& parentGroup, bool importable);

  static Result<> FinishImportingGridGeometryData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
