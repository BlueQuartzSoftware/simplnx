#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace nx::core
{
class IGridGeometry;

namespace HDF5
{
class SIMPLNX_EXPORT IGridGeometryIO : public IGeometryIO
{
public:
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
  static Result<> ReadGridGeometryData(DataStructureReader& dataStructureReader, IGridGeometry& geometry, const group_reader_type& parentGroup, const std::string& geometryName,
                                       DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the IGridGeometry data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteGridGeometryData(DataStructureWriter& dataStructureWriter, const IGridGeometry& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace nx::core
