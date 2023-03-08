#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/IGeometryIO.hpp"

namespace complex
{
class IGridGeometry;

namespace HDF5
{
class COMPLEX_EXPORT IGridGeometryIO : public IGeometryIO
{
public:
  IGridGeometryIO();
  virtual ~IGridGeometryIO() noexcept;

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
} // namespace complex
