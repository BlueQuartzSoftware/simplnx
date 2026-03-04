#pragma once

#include "simplnx/DataStructure/IO/HDF5/AbstractNodeGeom2dIO.hpp"

namespace nx::core
{
class QuadGeom;

namespace HDF5
{
class SIMPLNX_EXPORT QuadGeomIO : public AbstractNodeGeom2dIO
{
public:
  using data_type = QuadGeom;

  QuadGeomIO();
  ~QuadGeomIO() noexcept override;

  /**
   * @brief Attempts to read the QuadGeom from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& geomName, AbstractDataObject::IdType importId,
                    const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const override;

  /**
   * @brief Attempts to write an QuadGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const QuadGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the AbstractDataObject to HDF5.
   * Returns an error if the AbstractDataObject cannot be cast to a QuadGeom.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const override;

  AbstractDataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  QuadGeomIO(const QuadGeomIO& other) = delete;
  QuadGeomIO(QuadGeomIO&& other) = delete;
  QuadGeomIO& operator=(const QuadGeomIO& rhs) = delete;
  QuadGeomIO& operator=(QuadGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
