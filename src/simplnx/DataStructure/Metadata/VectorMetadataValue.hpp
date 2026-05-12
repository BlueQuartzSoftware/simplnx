#pragma once

#include "AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
template <typename T>
class SIMPLNX_EXPORT VectorMetadataValue : public AbstractMetadataValue<std::vector<T>>
{
public:
  using ParentType = AbstractMetadataValue<std::vector<T>>;
  using ValueType = ParentType::ValueType;

  static constexpr StringLiteral k_TypeName = "array";

  VectorMetadataValue(const ValueType& value)
  : ParentType()
  , m_Value(value)
  {
  }

  VectorMetadataValue(const VectorMetadataValue& other) = default;
  VectorMetadataValue(VectorMetadataValue&& other) = default;

  ~VectorMetadataValue() = default;

  /**
   * @brief Default cast to the type in question
   * @return metadata value
   */
  operator ValueType() const override
  {
    return m_Value;
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

  std::string getTypeName() const
  {
    return k_TypeName;
  }

protected:
  std::string toJsonImpl() const override
  {
    nlohmann::json json;
    json[k_ValueTypeKey] = k_TypeName;
    json[k_ValueKey] = m_Value;

    return json;
  }

  void fromJsonImpl(const std::string& jsonStr) override
  {
    nlohmann::json json(jsonStr);
    m_Value = json[k_ValueKey].get<ValueType>();
  }

private:
  std::vector<T> m_Value;
};
} // namespace nx::core
