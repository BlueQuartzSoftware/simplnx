#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <optional>

namespace complex
{
class IDataStructureReader;

/**
 * @class IDataFactory
 * @brief Base class for reading and writing DataObjects to file
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
