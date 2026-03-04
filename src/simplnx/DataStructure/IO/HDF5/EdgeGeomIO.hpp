#pragma once

#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry1D.hpp"
#include "simplnx/DataStructure/IO/HDF5/AbstractNodeGeom1dIO.hpp"

namespace nx::core
{
class EdgeGeom;

namespace HDF5
{
/**
 * @brief The EdgeGeomIO class exists to read and write EdgeGeoms using HDF5.
 */
class SIMPLNX_EXPORT EdgeGeomIO : public AbstractNodeGeom1dIO
{
public:
  using data_type = EdgeGeom;

  EdgeGeomIO();
  ~EdgeGeomIO() noexcept override;

  /**
   * @brief Attempts to read an EdgeGeom from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param geomName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& geomName, AbstractDataObject::IdType importId,
                    const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const override;

  /**
   * @brief Attempts to write an EdgeGeom to HDF5.
   * @param dataStructureWriter
   * @param geometry
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const EdgeGeom& geometry, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the AbstractDataObject to HDF5.
   * Returns an error if the AbstractDataObject cannot be cast to EdgeGeom.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const override;

  AbstractDataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  EdgeGeomIO(const EdgeGeomIO& other) = delete;
  EdgeGeomIO(EdgeGeomIO&& other) = delete;
  EdgeGeomIO& operator=(const EdgeGeomIO& rhs) = delete;
  EdgeGeomIO& operator=(EdgeGeomIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace nx::core
