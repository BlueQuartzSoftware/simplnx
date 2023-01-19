#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataFactoryManager.hpp"

#include <optional>

namespace complex
{
class DataFactoryManager;

namespace H5
{
class GroupReader;
class IDataFactory;

/**
 * @class DataStructureReader
 * @brief The DataStructureReader class manages the import process for a
 * DataStructure from HDF5. It controls whether or not an HDF5 object is
 * read from file or linked from an already imported DataObject.
 */
class COMPLEX_EXPORT DataStructureReader
{
public:
  /**
   * @brief Constructs a DataStructureReader with a sppecific DataFactoryManager.
   * If no DataFactoryManager is provided, the Application instance's is used
   * instead.
   * @param h5FactoryManager = nullptr
   */
  DataStructureReader(H5::DataFactoryManager* h5FactoryManager = nullptr);

  /**
   * @brief Deconstructs the DataStructureReader.
   */
  virtual ~DataStructureReader();

  /**
   * @brief Imports and returns a DataStructure from a target H5::GroupReader.
   * Returns any HDF5 error code that occur by reference. Otherwise, this value
   * is set to 0.
   * @param groupReader Target HDF5 group reader
   * @param errorCode HDF5 error code from reading the file.
   * @param preflight
   * @return complex::DataStructure
   */
  complex::DataStructure readH5Group(const H5::GroupReader& groupReader, H5::ErrorType& errorCode, bool preflight = false);

  /**
   * @brief Imports a complex::DataObject with the specified name from the target
   * HDF5 group. Returns any HDF5 error code that occurs. Returns 0 otherwise.
   * @param parentGroup HDF5 group reader for the parent DataMap
   * @param objectName Target complex::DataObject name
   * @param parentId = {} complex::DataObject parent ID
   * @param preflight
   * @return H5::ErrorType
   */
  H5::ErrorType readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false);

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
   * @return H5::DataFactoryManager*
   */
  H5::DataFactoryManager* getDataReader() const;

  /**
   * Returns a pointer to the appropriate H5::IDataFactory based on a target
   * datatype name.
   * @return H5::IDataFactory*
   */
  H5::IDataFactory* getDataFactory(const std::string& typeName) const;

private:
  H5::DataFactoryManager* m_FactoryManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace H5
} // namespace complex
