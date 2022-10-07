#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <optional>

namespace complex
{
class IDataStructureReader;

/**
 * @class IDataFactory
 * @brief
 */
class COMPLEX_EXPORT IDataFactory
{
public:
  virtual ~IDataFactory() noexcept = default;

  /**
   * @brief Returns the Type of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual DataObject::Type getDataType() const = 0;

#if 0
  /**
   * @brief Creates and adds a DataObject using the provided DataStructure,
   * parent reader, and object reader.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @param useEmptyDataStores = false
   * @return Returns a valid Result<> if the process succeeded. Otherwise,
   * this method returns an invalid Result<> specifying the encountered errors.
   */
  virtual Result<> readDataObject(IDataStructureReader& dataStructureReader, const std::any& parentReader, const std::any& objectReader,
                                  const std::optional<complex::DataObject::IdType>& parentId = {}, bool useEmptyDataStores = false) = 0;
#endif

  // Copy and move constuctors / operators deleted
  IDataFactory(const IDataFactory& other) = delete;
  IDataFactory(IDataFactory&& other) = delete;
  IDataFactory& operator=(const IDataFactory& rhs) = delete;
  IDataFactory& operator=(IDataFactory&& rhs) = delete;

protected:
  /**
   * @brief Default constructor
   */
  IDataFactory() = default;
};
} // namespace complex
