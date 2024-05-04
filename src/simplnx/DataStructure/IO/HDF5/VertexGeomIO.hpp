#pragma once

#include "simplnx/DataStructure/IO/HDF5/INodeGeom0dIO.hpp"

namespace nx::core
{
class VertexGeom;

namespace HDF5
{
class SIMPLNX_EXPORT VertexGeomIO : public INodeGeom0dIO
{
public:
  using data_type = VertexGeom;

  VertexGeomIO();
  ~VertexGeomIO() noexcept override;

  /**
   * @brief Attempts to read the VertexGeom from HDF5.
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
   * @brief Attempts to write an VertexGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const VertexGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a VertexGeom.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  VertexGeomIO(const VertexGeomIO& other) = delete;
  VertexGeomIO(VertexGeomIO&& other) = delete;
  VertexGeomIO& operator=(const VertexGeomIO& rhs) = delete;
  VertexGeomIO& operator=(VertexGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
