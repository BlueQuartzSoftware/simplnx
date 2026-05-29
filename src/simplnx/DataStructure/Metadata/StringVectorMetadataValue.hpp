#pragma once

#include "AbstractVectorMetadataValue.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT StringVectorMetadataValue : public AbstractVectorMetadataValue<std::string>
{
public:
  using ParentType = AbstractVectorMetadataValue<std::string>;
  using ValueType = typename ParentType::ValueType;
  using AssignmentReturnType = typename ParentType::AssignmentReturnType;

  static constexpr StringLiteral k_TypeName = "vec<string>";

  StringVectorMetadataValue();
  StringVectorMetadataValue(const StringVectorMetadataValue& rhs);
  StringVectorMetadataValue(StringVectorMetadataValue&& rhs) noexcept;
  ~StringVectorMetadataValue() noexcept = default;

  StringVectorMetadataValue& operator=(const StringVectorMetadataValue& rhs) = default;
  StringVectorMetadataValue& operator=(StringVectorMetadataValue&& rhs) noexcept = default;

  /**
   * @brief Assignment operator
   * @param rhs
   */
  AssignmentReturnType& operator=(const ValueType& rhs) override;

  /**
   * @brief Default equality operator
   * @param rhs value to compare against
   * @return is equal
   */
  bool operator==(const ValueType& rhs) const override;

protected:
  std::string getTypeNameImpl() const override;
};
} // namespace nx::core
