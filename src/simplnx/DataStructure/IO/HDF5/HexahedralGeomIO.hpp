#pragma once

#include "simplnx/DataStructure/IO/HDF5/INodeGeom3dIO.hpp"

namespace nx::core
{
class HexahedralGeom;

namespace HDF5
{
class SIMPLNX_EXPORT HexahedralGeomIO : public INodeGeom3dIO
{
public:
  using data_type = HexahedralGeom;

  /**
   * @brief Default constructor.
   */
  HexahedralGeomIO();
  ~HexahedralGeomIO() noexcept override;

  /**
   * @brief Attempts to read the HexahedralGeom from HDF5.
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
   * @brief Finishes importing HexahedralGeom data after the geometry has been created.
   * @param dataStructure The DataStructure containing the imported geometry
   * @param dataPath The path to the imported HexahedralGeom
   * @param dataStructureGroup The HDF5 group containing the geometry data
   * @return Result<> Result with any errors or warnings
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const override;

  /**
   * @brief Attempts to write a HexahedralGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const HexahedralGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a HexahedralGeom.
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

  HexahedralGeomIO(const HexahedralGeomIO& other) = delete;
  HexahedralGeomIO(HexahedralGeomIO&& other) = delete;
  HexahedralGeomIO& operator=(const HexahedralGeomIO& rhs) = delete;
  HexahedralGeomIO& operator=(HexahedralGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
