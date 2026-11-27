#pragma once

#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace nx::core
{
class DataGroup;

namespace hdf5
{
/**
 * @brief The DataGroupIO class serves as the basis for writing DataGroups to HDF5.
 */
class SIMPLNX_EXPORT DataGroupIO : public BaseGroupIO
{
public:
  using data_type = DataGroup;

  /**
   * @brief Default constructor.
   */
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
  Result<> readData(DataStructureReader& dataStructureReader, const GroupReaderType& parentGroup, const std::string& dataGroupName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;

  /**
   * @brief Attempts to write a DataGroup to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param dataGroup
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> writeData(DataStructureWriter& dataStructureWriter, const DataGroup& dataGroup, GroupWriterType& parentGroup, bool importable);

  /**
   * @brief Attempts to write the target DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a DataGroup.
   * Otherwise, this method returns the result of writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const override;

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
};
} // namespace hdf5
} // namespace nx::core
