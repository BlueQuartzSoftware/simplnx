#pragma once

#include "BaseMetadataValue.hpp"

#include "simplnx/Common/StringLiteral.hpp"

#include <string>

namespace nx::core
{
/**
 * @brief Abstract typed meta data value class. Derived classes implement the
 * specifics for reading and writing the target value, but the AbstractMetaData
 * class allows reading and assigning the appropriate data type.
 */
template <typename T>
class AbstractMetadataValue : public BaseMetadataValue
{
public:
  using ValueType = T;

  ~AbstractMetadataValue() noexcept = default;

  /**
   * @brief Default cast to the type in question
   * @return metadata value
   */
  virtual operator ValueType() const = 0;

  /**
   * @brief Assignment operator
   * @param rhs
   */
  virtual AbstractMetadataValue& operator=(const ValueType& rhs) = 0;

  virtual std::string getTypeName() const = 0;

  /**
   * @brief Returns a json string representation for the meta data.
   * @return json string
   */
  std::string toJson() const override
  {
    return toJsonImpl();
  }

  /**
   * @brief Reads and updates the meta data value based on the provided json string
   * @param jsonStr json string value
   */
  void fromJson(const std::string& jsonStr) override
  {
    return fromJsonImpl(jsonStr);
  }

protected:
  AbstractMetadataValue() = default;
  AbstractMetadataValue(const AbstractMetadataValue& other) = default;
  AbstractMetadataValue(AbstractMetadataValue&& other) = default;

  /**
   * @brief Abstract method for derived classes to specify the appropriate json string.
   * @return json string
   */
  virtual std::string toJsonImpl() const = 0;

  /**
   * @brief Abstract method for reading data from a json string.
   * @param jsonStr json string
   */
  virtual void fromJsonImpl(const std::string& jsonStr) = 0;
};
} // namespace nx::core
