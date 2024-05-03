#pragma once

#include "simplnx/DataStructure/IO/HDF5/INodeGeom3dIO.hpp"

namespace nx::core
{
class TetrahedralGeom;

namespace HDF5
{
class SIMPLNX_EXPORT TetrahedralGeomIO : public INodeGeom3dIO
{
public:
  using data_type = TetrahedralGeom;

  TetrahedralGeomIO();
  ~TetrahedralGeomIO() noexcept override;

  /**
   * @brief Attempts to read the TetrahedralGeom from HDF5.
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
   * @brief Attempts to write an TetrahedralGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const TetrahedralGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a TetrahedralGeom.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  TetrahedralGeomIO(const TetrahedralGeomIO& other) = delete;
  TetrahedralGeomIO(TetrahedralGeomIO&& other) = delete;
  TetrahedralGeomIO& operator=(const TetrahedralGeomIO& rhs) = delete;
  TetrahedralGeomIO& operator=(TetrahedralGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
