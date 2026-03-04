#pragma once

#include "simplnx/DataStructure/IO/HDF5/AbstractDataIO.hpp"

namespace nx::core
{
class StringArray;

namespace HDF5
{
/**
 * @brief The StringArrayIO class serves as a reader and writer between StringArrays and HDF5
 */
class SIMPLNX_EXPORT StringArrayIO : public AbstractDataIO
{
public:
  using data_type = StringArray;

  StringArrayIO();
  ~StringArrayIO() noexcept override;

  StringArrayIO(const StringArrayIO& other) = delete;
  StringArrayIO(StringArrayIO&& other) = delete;
  StringArrayIO& operator=(const StringArrayIO& rhs) = delete;
  StringArrayIO& operator=(StringArrayIO&& rhs) = delete;

  /**
   * @brief Attempts to read the StringArray from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param arrayName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& arrayName, AbstractDataObject::IdType importId,
                    const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore) const override;

  /**
   * @brief Attempts to write an StringArray to HDF5.
   * @param dataStructureWriter
   * @param stringArray
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const data_type& stringArray, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Replaces the AbstractDataStore using data from the HDF5 dataset.
   * @param dataStructure
   * @param dataPath
   * @param dataStructureReader
   * @return Result<>
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroupReader) const override;

  /**
   * @brief Attempts to write the AbstractDataObject to HDF5.
   * Returns an error if the AbstractDataObject cannot be cast to a StringArray.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const override;

  AbstractDataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace nx::core
