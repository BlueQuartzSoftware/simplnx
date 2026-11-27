#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace nx::core
{
class IGridGeometry;

namespace hdf5
{
class SIMPLNX_EXPORT IGridGeometryIO : public IGeometryIO
{
public:
  /**
   * @brief Protected constructor.
   */
  IGridGeometryIO();
  ~IGridGeometryIO() noexcept override;

  IGridGeometryIO(const IGridGeometryIO& other) = delete;
  IGridGeometryIO(IGridGeometryIO&& other) = delete;
  IGridGeometryIO& operator=(const IGridGeometryIO& rhs) = delete;
  IGridGeometryIO& operator=(IGridGeometryIO&& rhs) = delete;

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
  static Result<> ReadGridGeometryData(DataStructureReader& dataStructureReader, IGridGeometry& geometry, const GroupReaderType& parentGroup, const std::string& geometryName,
                                       DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the IGridGeometry data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const IGridGeometry& geometry, GroupWriterType& parentGroup, bool importable);

  /**
   * @brief Finishes importing grid geometry data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported geometry
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  static Result<> FinishImportingGridGeometryData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& dataStructureGroup);
};
} // namespace hdf5
} // namespace nx::core
