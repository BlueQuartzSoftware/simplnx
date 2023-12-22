#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <optional>

namespace nx::core
{
class IDataStructureReader;

/**
 * @class IDataFactory
 * @brief Base class for reading and writing DataObjects to file
 */
class SIMPLNX_EXPORT IDataFactory
{
public:
  virtual ~IDataFactory() noexcept = default;

  /**
   * @brief Returns the Type of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual DataObject::Type getDataType() const = 0;

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
} // namespace nx::core
