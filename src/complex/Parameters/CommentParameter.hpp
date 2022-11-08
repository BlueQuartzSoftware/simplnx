#pragma once

#include <string>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT CommentParameter : public ValueParameter
{
public:
  using ValueType = std::string;

  CommentParameter() = delete;
  CommentParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~CommentParameter() override = default;

  CommentParameter(const CommentParameter&) = delete;
  CommentParameter(CommentParameter&&) noexcept = delete;

  CommentParameter& operator=(const CommentParameter&) = delete;
  CommentParameter& operator=(CommentParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief
   * @return
   */
  ValueType defaultString() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::CommentParameter, "e1bf40d9-a3d4-4273-85b0-7da4a79e2382");
