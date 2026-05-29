#pragma once

#include "AbstractMetadataValue.hpp"

namespace nx::core
{
template <typename T>
class AbstractVectorMetadataValue : public AbstractMetadataValue<std::vector<T>>
{
public:
  using ParentType = typename AbstractMetadataValue<std::vector<T>>;
  using ValueType = typename ParentType::ValueType;
  using AssignmentReturnType = typename ParentType;

  AbstractVectorMetadataValue(const AbstractVectorMetadataValue& other)
  : ParentType(other)
  , m_Value(other.m_Value)
  {
  }
  AbstractVectorMetadataValue(AbstractVectorMetadataValue&& other) noexcept
  : ParentType(other)
  , m_Value(std::move(other.m_Value))
  {
  }

  ~AbstractVectorMetadataValue() noexcept = default;

  /**
   * @brief Default cast to the type in question
   * @return metadata value
   */
  operator ValueType() const override
  {
    return m_Value;
  }

  /**
   * @brief Returns the stored value.
   * @return metadata value
   */
  ValueType getValue() const override
  {
    return m_Value;
  }

  /**
   * @brief Sets the stored value.
   * @param value
   */
  void setValue(const ValueType& value) override
  {
    m_Value = value;
  }

  /**
   * @brief Assignment operator
   * @param rhs
   */
  ParentType& operator=(const ValueType& rhs) override
  {
    m_Value = rhs;
    return *this;
  }

  /**
   * @brief Default equality operator
   * @param rhs value to compare against
   * @return is equal
   */
  bool operator==(const ValueType& rhs) const override
  {
    return m_Value == rhs;
  }

protected:
  AbstractVectorMetadataValue()
  : ParentType()
  {
  }

  AbstractVectorMetadataValue& operator=(const AbstractVectorMetadataValue& rhs)
  {
    m_Value = rhs.m_Value;
    return *this;
  }
  AbstractVectorMetadataValue& operator=(AbstractVectorMetadataValue&& rhs) noexcept
  {
    m_Value = std::move(rhs.m_Value);
    return *this;
  }

  nlohmann::json toJsonImpl() const override
  {
    nlohmann::json json;
    json[ParentType::k_ValueTypeKey] = ParentType::getTypeName();
    json[ParentType::k_ValueKey] = m_Value;
    return json;
  }

  void fromJsonImpl(const nlohmann::json& json) override
  {
    m_Value = json[ParentType::k_ValueKey].get<ValueType>();
  }

private:
  std::vector<T> m_Value;
};
} // namespace nx::core
