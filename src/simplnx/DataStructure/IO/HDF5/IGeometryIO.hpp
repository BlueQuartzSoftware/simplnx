#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class IGeometry;

namespace HDF5
{
/**
 * @brief Base class for simplnx geometry IO using HDF5
 */
class SIMPLNX_EXPORT IGeometryIO : public BaseGroupIO
{
public:
  /**
   * @brief Protected constructor.
   */
  IGeometryIO();
  virtual ~IGeometryIO() noexcept;

  IGeometryIO(const IGeometryIO& other) = delete;
  IGeometryIO(IGeometryIO&& other) = delete;
  IGeometryIO& operator=(const IGeometryIO& rhs) = delete;
  IGeometryIO& operator=(IGeometryIO&& rhs) = delete;

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
  static Result<> ReadGeometryData(DataStructureReader& dataStructureReader, IGeometry& geometry, const group_reader_type& parentGroup, const std::string& geometryName, DataObject::IdType importId,
                                   const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Writes the generic geometry data to HDF5.
   * Returns a Result with any errors or warnings that were encountered during the export process.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteGeometryData(DataStructureWriter& dataStructureWriter, const IGeometry& geometry, group_writer_type& parentGroup, bool importable);

  /**
   * @brief Finishes importing geometry data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported geometry
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  static Result<> FinishImportingGeomData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup);
};
} // namespace HDF5
} // namespace nx::core
