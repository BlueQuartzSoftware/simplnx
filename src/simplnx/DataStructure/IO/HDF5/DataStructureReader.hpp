#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core::HDF5
{
class IDataIO;
class DataIOManager;

/**
 * @brief The DataStructureReader class exists to read DataStructures from an HDF5 file or group.
 */
class SIMPLNX_EXPORT DataStructureReader
{
public:
  /**
   * @brief Constructs a DataStructureReader with an optional IO manager.
   * @param ioManager Optional pointer to a custom DataIOManager. If nullptr, uses the default manager.
   */
  DataStructureReader(DataIOManager* ioManager = nullptr);
  ~DataStructureReader() noexcept;

  /**
   * @brief Attempts to read a DataStructure from the corresponding file path.
   * @param path
   * @param useEmptyDataStores = false
   * @return Result<DataStructure>
   */
  static Result<DataStructure> ReadFile(const std::filesystem::path& path, bool useEmptyDataStores = false);

  /**
   * @brief Attempts to read a DataStructure from the corresponding HDF5 file.
   * @param fileReader
   * @param useEmptyDataStores = false
   * @return Result<DataStructure>
   */
  static Result<DataStructure> ReadFile(const nx::core::HDF5::FileIO& fileReader, bool useEmptyDataStores = false);

  /**
   * @brief Reads a single DataObject from an HDF5 file at the specified path.
   * @param fileReader The HDF5 file reader to read from
   * @param dataPath The path to the DataObject to read
   * @return Result<std::shared_ptr<DataObject>> The read DataObject or an error
   */
  static Result<std::shared_ptr<DataObject>> ReadObject(const nx::core::HDF5::FileIO& fileReader, const DataPath& dataPath);

  /**
   * @brief Finishes importing a DataObject after it has been read and added to the DataStructure.
   * @param dataStructure The DataStructure containing the imported object
   * @param fileReader The HDF5 file reader
   * @param dataPath The path to the imported DataObject
   * @return Result<> Result with any errors or warnings
   */
  static Result<> FinishImportingObject(DataStructure& dataStructure, const nx::core::HDF5::FileIO& fileReader, const DataPath& dataPath);

  /**
   * @brief Imports and returns a DataStructure from a target nx::core::HDF5::GroupIO.
   * Returns any HDF5 error code that occur by reference. Otherwise, this value
   * is set to 0.
   * @param groupReader Target HDF5 group reader
   * @param useEmptyDataStores = false
   * @return Result<DataStructure>
   */
  Result<DataStructure> readGroup(const nx::core::HDF5::GroupIO& groupReader, bool useEmptyDataStores = false);

  /**
   * @brief Imports a DataObject with the specified name from the target
   * HDF5 group. Returns any HDF5 error code that occurs. Returns 0 otherwise.
   * @param parentGroup HDF5 group reader for the parent DataMap
   * @param objectName Target DataObject name
   * @param parentId = {} DataObject parent ID
   * @param useEmptyDataStores = false
   * @return Result<>
   */
  Result<> readObjectFromGroup(const nx::core::HDF5::GroupIO& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {}, bool useEmptyDataStores = false);

  /**
   * @brief Returns a reference to the current DataStructure. Returns an empty
   * DataStructure when not importing from HDF5 file.
   * @return DataStructure&
   */
  DataStructure& getDataStructure();

  /**
   * @brief Resets the current DataStructure.
   */
  void clearDataStructure();

protected:
  /**
   * @brief Returns a pointer to the nx::core::HDF5::DataFactoryManager used for finding the
   * appropriate. If one was not provided in the constructor, this returns the
   * Application instance's nx::core::HDF5::DataFactoryManager.
   * @return std::shared_ptr<DataIOManager>
   */
  std::shared_ptr<DataIOManager> getDataReader() const;

  /**
   * @brief Returns a pointer to the appropriate nx::core::HDF5::IDataFactory based on a target data type.
   * @param typeName The type name of the data factory to retrieve
   * @return std::shared_ptr<IDataIO> Pointer to the data factory
   */
  std::shared_ptr<IDataIO> getDataFactory(typename IDataIOManager::factory_id_type typeName) const;

private:
  std::shared_ptr<DataIOManager> m_IOManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace nx::core::HDF5
