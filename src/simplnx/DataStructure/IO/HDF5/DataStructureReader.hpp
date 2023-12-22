#pragma once

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

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
  static Result<DataStructure> ReadFile(const nx::core::HDF5::FileReader& fileReader, bool useEmptyDataStores = false);

  /**
   * @brief Imports and returns a DataStructure from a target nx::core::HDF5::GroupReader.
   * Returns any HDF5 error code that occur by reference. Otherwise, this value
   * is set to 0.
   * @param groupReader Target HDF5 group reader
   * @param useEmptyDataStores = false
   * @return Result<DataStructure>
   */
  Result<DataStructure> readGroup(const nx::core::HDF5::GroupReader& groupReader, bool useEmptyDataStores = false);

  /**
   * @brief Imports a DataObject with the specified name from the target
   * HDF5 group. Returns any HDF5 error code that occurs. Returns 0 otherwise.
   * @param parentGroup HDF5 group reader for the parent DataMap
   * @param objectName Target DataObject name
   * @param parentId = {} DataObject parent ID
   * @param useEmptyDataStores = false
   * @return Result<>
   */
  Result<> readObjectFromGroup(const nx::core::HDF5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {}, bool useEmptyDataStores = false);

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
   * Returns a pointer to the appropriate nx::core::HDF5::IDataFactory based on a target
   * data type.
   * @return std::shared_ptr<IDataIO>
   */
  std::shared_ptr<IDataIO> getDataFactory(typename IDataIOManager::factory_id_type typeName) const;

private:
  std::shared_ptr<DataIOManager> m_IOManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace nx::core::HDF5
