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

  /**
   * @brief Default constructor.
   */
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
   * @brief Finishes importing VertexGeom data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported VertexGeom
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const override;

  /**
   * @brief Attempts to write a VertexGeom to HDF5.
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

  VertexGeomIO(const VertexGeomIO& other) = delete;
  VertexGeomIO(VertexGeomIO&& other) = delete;
  VertexGeomIO& operator=(const VertexGeomIO& rhs) = delete;
  VertexGeomIO& operator=(VertexGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
