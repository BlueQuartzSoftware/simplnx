#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/INodeGeom0dIO.hpp"

namespace complex
{
class INodeGeometry1D;

namespace HDF5
{
class COMPLEX_EXPORT INodeGeom1dIO : public INodeGeom0dIO
{
public:
  INodeGeom1dIO();
  virtual ~INodeGeom1dIO() noexcept;

  INodeGeom1dIO(const INodeGeom1dIO& other) = delete;
  INodeGeom1dIO(INodeGeom1dIO&& other) = delete;
  INodeGeom1dIO& operator=(const INodeGeom1dIO& rhs) = delete;
  INodeGeom1dIO& operator=(INodeGeom1dIO&& rhs) = delete;

protected:
  /**
   * @brief Attempts to read the INodeGeometry1D data from HDF5.
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
  static Result<> ReadNodeGeom1dData(DataStructureReader& dataStructureReader, INodeGeometry1D& geometry, const group_reader_type& parentGroup, const std::string& geomName,
                                     DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief Attempts to write the INodeGeometry1D data to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteNodeGeom1dData(DataStructureWriter& dataStructureWriter, const INodeGeometry1D& geometry, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
