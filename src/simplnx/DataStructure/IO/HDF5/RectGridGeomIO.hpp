#pragma once

#include "simplnx/DataStructure/IO/HDF5/IGridGeometryIO.hpp"

namespace nx::core
{
class RectGridGeom;

namespace HDF5
{
class SIMPLNX_EXPORT RectGridGeomIO : public IGridGeometryIO
{
public:
  using data_type = RectGridGeom;

  /**
   * @brief Validates available rectilinear grid bounds after their arrays have been imported.
   * @param dataStructure Contains the imported geometry objects and their bounds arrays.
   * @param filePath Identifies the source file.
   * @return The first bounds validation error, with its geometry name and file path.
   */
  [[nodiscard]] static Result<> ValidateImportedBounds(const DataStructure& dataStructure, const std::filesystem::path& filePath);

  /**
   * @brief Validates available rectilinear grid bounds for the geometries at the specified paths.
   * @param dataStructure Contains the geometry objects and their bounds arrays.
   * @param filePath Identifies the source file.
   * @param selectedPaths Identifies just the objects being imported.
   * @return The first bounds validation error, with its geometry name and file path.
   */
  [[nodiscard]] static Result<> ValidateImportedBounds(const DataStructure& dataStructure, const std::filesystem::path& filePath, const std::vector<DataPath>& selectedPaths);

  /**
   * @brief Default constructor.
   */
  RectGridGeomIO();
  ~RectGridGeomIO() noexcept override;

  /**
   * @brief Attempts to read the RectGridGeom from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& geomName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Finishes importing RectGridGeom data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported RectGridGeom
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const override;

  /**
   * @brief Attempts to write a RectGridGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> writeData(DataStructureWriter& dataStructureWriter, const RectGridGeom& geometry, group_writer_type& parentGroup, bool importable) ;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a RectGridGeom.
   * Otherwise, this method returns writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   * @return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  /**
   * @brief Returns the DataObject::Type for this IO class.
   * @return DataObject::Type The type identifier
   */
  DataObject::Type getDataType() const override;

  /**
   * @brief Returns the DataObject type name as a string for this IO class.
   * @return std::string The type name
   */
  std::string getTypeName() const override;

  RectGridGeomIO(const RectGridGeomIO& other) = delete;
  RectGridGeomIO(RectGridGeomIO&& other) = delete;
  RectGridGeomIO& operator=(const RectGridGeomIO& rhs) = delete;
  RectGridGeomIO& operator=(RectGridGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
