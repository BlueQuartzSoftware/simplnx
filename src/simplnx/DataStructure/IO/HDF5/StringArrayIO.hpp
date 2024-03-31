#pragma once

#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"

namespace nx::core
{
class StringArray;

namespace HDF5
{
/**
 * @brief The StringArrayIO class serves as a reader and writer between StringArrays and HDF5
 */
class SIMPLNX_EXPORT StringArrayIO : public IDataIO
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
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& arrayName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const override;

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
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a StringArray.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace nx::core
