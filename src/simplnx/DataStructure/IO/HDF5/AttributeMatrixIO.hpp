#pragma once

#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class AttributeMatrix;

namespace HDF5
{
/**
 * @brief AttributeMatrix reader / writer class using HDF5.
 */
class SIMPLNX_EXPORT AttributeMatrixIO : public BaseGroupIO
{
public:
  using data_type = AttributeMatrix;

  AttributeMatrixIO();
  virtual ~AttributeMatrixIO() noexcept;

  AttributeMatrixIO(const AttributeMatrixIO& other) = delete;
  AttributeMatrixIO(AttributeMatrixIO&& other) = delete;
  AttributeMatrixIO& operator=(const AttributeMatrixIO& rhs) = delete;
  AttributeMatrixIO& operator=(AttributeMatrixIO&& rhs) = delete;

  /**
   * @brief Attempts to read the AttributeMatrix in from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the import process.
   * @param dataStructureReader
   * @param parentGroup
   * @param attributeMatrixName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& attributeMatrixName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Attempts to write the AttributeMatrix to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the export process.
   * @param dataStructureWriter
   * @param dataGroup
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const data_type& dataGroup, group_writer_type& parentGroup, bool importable) const;

  /**
   * @param Attempts to write the AttributeMatrix to HDF5.
   * This method is returns an error if DataObject cannot be cast to an AttributeMatrix.
   * Otherwise, this method returns the result of writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  /**
   * @brief Returns the DataObject::Type for this IO class.
   * @return DataObject::Type
   */
  DataObject::Type getDataType() const override;

  /**
   * @brief Returns the DataObject type name as a string for this IO class.
   * @return std::string
   */
  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace nx::core
