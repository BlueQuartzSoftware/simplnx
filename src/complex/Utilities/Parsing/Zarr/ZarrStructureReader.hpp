#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/Zarr/Zarr.hpp"

#include "complex/complex_export.hpp"

#include <optional>
#include <string>

namespace complex
{
namespace Zarr
{
class DataFactoryManager;
class IDataFactory;

class COMPLEX_EXPORT DataStructureReader
{
public:
  /**
   * @brief Constructs a DataStructureReader with a sppecific DataFactoryManager.
   * If no DataFactoryManager is provided, the Application instance's is used
   * instead.
   * @param factoryManager = nullptr
   */
  DataStructureReader(Zarr::DataFactoryManager* factoryManager = nullptr);

  /**
   * @brief Deconstructs the DataStructureReader.
   */
  virtual ~DataStructureReader();

  /**
   * @brief Imports and returns a DataStructure from a target FileVec::Group.
   * Returns any HDF5 error code that occur by reference. Otherwise, this value
   * is set to 0.
   * @param groupReader Target HDF5 group reader
   * @param errorCode HDF5 error code from reading the file.
   * @param preflight
   * @return complex::DataStructure
   */
  complex::DataStructure readGroup(const FileVec::IGroup& groupReader, Zarr::ErrorType& errorCode, bool preflight = false);

  /**
   * @brief Imports a complex::DataObject with the specified name from the target
   * HDF5 group. Returns any HDF5 error code that occurs. Returns 0 otherwise.
   * @param parentGroup HDF5 group reader for the parent DataMap
   * @param objectName Target complex::DataObject name
   * @param parentId = {} complex::DataObject parent ID
   * @param preflight
   * @return Zarr::ErrorType
   */
  Zarr::ErrorType readObjectFromGroup(const FileVec::IGroup& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false);

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
   * @brief Returns a pointer to the Zarr::DataFactoryManager used for finding the
   * appropriate. If one was not provided in the constructor, this returns the
   * Application instance's Zarr::DataFactoryManager.
   * @return Zarr::DataFactoryManager*
   */
  Zarr::DataFactoryManager* getDataReader() const;

  /**
   * Returns a pointer to the appropriate Zarr::IDataFactory based on a target
   * datatype name.
   * @return Zarr::IDataFactory*
   */
  Zarr::IDataFactory* getDataFactory(const std::string& typeName) const;

private:
  Zarr::DataFactoryManager* m_FactoryManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace Zarr
} // namespace complex
