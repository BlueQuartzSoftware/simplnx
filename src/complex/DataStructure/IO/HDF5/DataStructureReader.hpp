#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
namespace HDF5
{
class GroupReader;
class IDataIO;
class DataIOManager;

class COMPLEX_EXPORT DataStructureReader
{
public:
  DataStructureReader(DataIOManager* ioManager = nullptr);
  ~DataStructureReader() noexcept;

  Result<DataStructure> readFile(const std::filesystem::path& path);
  Result<DataStructure> readFile(const H5::FileReader& fileReader);

  /**
   * @brief Imports and returns a DataStructure from a target H5::GroupReader.
   * Returns any HDF5 error code that occur by reference. Otherwise, this value
   * is set to 0.
   * @param groupReader Target HDF5 group reader
   * @param errorCode HDF5 error code from reading the file.
   * @param useEmptyDataStores
   * @return complex::DataStructure
   */
  Result<DataStructure> readGroup(const H5::GroupReader& groupReader, bool useEmptyDataStores = false);

  /**
   * @brief Imports a complex::DataObject with the specified name from the target
   * HDF5 group. Returns any HDF5 error code that occurs. Returns 0 otherwise.
   * @param parentGroup HDF5 group reader for the parent DataMap
   * @param objectName Target complex::DataObject name
   * @param parentId = {} complex::DataObject parent ID
   * @param useEmptyDataStores = false
   * @return Result<>
   */
  Result<> readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {}, bool useEmptyDataStores = false);

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
   * @brief Returns a pointer to the H5::DataFactoryManager used for finding the
   * appropriate. If one was not provided in the constructor, this returns the
   * Application instance's H5::DataFactoryManager.
   * @return std::shared_ptr<DataIOManager>
   */
  std::shared_ptr<DataIOManager> getDataReader() const;

  /**
   * Returns a pointer to the appropriate H5::IDataFactory based on a target
   * data type.
   * @return std::shared_ptr<IDataIO>
   */
  std::shared_ptr<IDataIO> getDataFactory(typename IDataIOManager::factory_id_type typeName) const;

private:
  std::shared_ptr<DataIOManager> m_IOManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace HDF5
} // namespace complex
