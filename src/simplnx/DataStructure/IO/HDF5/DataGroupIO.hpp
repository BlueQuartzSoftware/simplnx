#pragma once

#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class DataGroup;

namespace HDF5
{
/**
 * @brief The DataGroupIO class serves as the basis for writing DataGroups to HDF5.
 */
class SIMPLNX_EXPORT DataGroupIO : public BaseGroupIO
{
public:
  using data_type = DataGroup;

  DataGroupIO();
  ~DataGroupIO() noexcept override;

  DataGroupIO(const DataGroupIO& other) = delete;
  DataGroupIO(DataGroupIO&& other) = delete;
  DataGroupIO& operator=(const DataGroupIO& rhs) = delete;
  DataGroupIO& operator=(DataGroupIO&& rhs) = delete;

  /**
   * @brief Attempts to read the target DataGroup from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param dataGroupName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& dataGroupName, AbstractDataObject::IdType importId,
                    const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Attempts to write a DataGroup to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param dataGroup
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const DataGroup& dataGroup, group_writer_type& parentGroup, bool importable) const;

  /**
   * @brief Attempts to write the target AbstractDataObject to HDF5.
   * Returns an error if the AbstractDataObject cannot be cast to a DataGroup.
   * Otherwise, this method returns the result of writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter) const override;

  AbstractDataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace nx::core
