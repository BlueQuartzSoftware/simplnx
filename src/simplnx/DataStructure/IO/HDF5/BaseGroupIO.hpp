#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"

namespace nx::core
{
class BaseGroup;

namespace HDF5
{
/**
 * @brief The BaseGroupIO class serves as the base IO class for reading and writing BaseGroups to HDF5
 */
class SIMPLNX_EXPORT BaseGroupIO : public IDataIO
{
public:
  BaseGroupIO();
  virtual ~BaseGroupIO() noexcept;

  BaseGroupIO(const BaseGroupIO& other) = delete;
  BaseGroupIO(BaseGroupIO&& other) = delete;
  BaseGroupIO& operator=(const BaseGroupIO& rhs) = delete;
  BaseGroupIO& operator=(BaseGroupIO&& rhs) = delete;

  /**
   * @brief Attempts to write the DataMap to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param dataMap
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteDataMap(DataStructureWriter& dataStructureWriter, const DataMap& dataMap, group_writer_type& parentGroup, bool importable);

protected:
  /**
   * @brief Attempts to read the BaseGroup data from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param baseGroup
   * @param parentGroup
   * @param objectName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore
   * @return Result<>
   */
  static Result<> ReadBaseGroupData(DataStructureReader& dataStructureReader, BaseGroup& baseGroup, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  /**
   * @brief attempts to write the BaseGroup data to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param baseGroup
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  static Result<> WriteBaseGroupData(DataStructureWriter& dataStructureWriter, const BaseGroup& baseGroup, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace nx::core
