#pragma once

#include "BaseMetadataValue.hpp"

#include "simplnx/Common/StringLiteral.hpp"

#include <nlohmann/json.hpp>

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

  AbstractMetadataValue(const AbstractMetadataValue& other) = default;
  AbstractMetadataValue(AbstractMetadataValue&& other) noexcept = default;
  ~AbstractMetadataValue() noexcept = default;

  /**
   * @brief Default cast to the type in question
   * @return metadata value
   */
  virtual operator ValueType() const = 0;

  /**
   * @brief Returns the stored value.
   * @return metadata value
   */
  virtual ValueType getValue() const = 0;

  /**
   * @brief Sets the stored value.
   * @param value
   */
  virtual void setValue(const ValueType& value) = 0;

  /**
   * @brief Assignment operator
   * @param rhs
   */
  virtual AbstractMetadataValue& operator=(const ValueType& rhs) = 0;

  /**
   * @brief Default equality operator
   * @param rhs value to compare against
   * @return is equal
   */
  virtual bool operator==(const ValueType& rhs) const = 0;

  std::string getTypeName() const override
  {
    return getTypeNameImpl();
  }

  /**
   * @brief Returns a json string representation for the meta data.
   * @return json
   */
  nlohmann::json toJson() const override
  {
    return toJsonImpl();
  }

  /**
   * @brief Reads and updates the meta data value based on the provided json string
   * @param json
   */
  void fromJson(const nlohmann::json& json) override
  {
    return fromJsonImpl(json);
  }

protected:
  AbstractMetadataValue() = default;

  virtual std::string getTypeNameImpl() const = 0;

  /**
   * @brief Abstract method for derived classes to specify the appropriate json.
   * @return json
   */
  virtual nlohmann::json toJsonImpl() const = 0;

  /**
   * @brief Abstract method for reading data from a json.
   * @param jsonStr json
   */
  virtual void fromJsonImpl(const nlohmann::json& json) = 0;
};
} // namespace nx::core
