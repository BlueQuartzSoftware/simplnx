#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Filter/AbstractParameter.hpp"

namespace complex
{
/**
 * @brief ValueParameter provides an interface for parameters that deal with value types (e.g. integer, string, etc.).
 */
class COMPLEX_EXPORT ValueParameter : public AbstractParameter
{
public:
  ~ValueParameter() noexcept override = default;

  ValueParameter(const ValueParameter&) = delete;
  ValueParameter(ValueParameter&&) noexcept = delete;

  ValueParameter& operator=(const ValueParameter&) = delete;
  ValueParameter& operator=(ValueParameter&&) noexcept = delete;

  /**
   * @brief Returns whether the parameter is a ValueParameter or DataParameter.
   * @return
   */
  [[nodiscard]] Type type() const final;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return
   */
  [[nodiscard]] virtual Result<> validate(const std::any& value) const = 0;

protected:
  ValueParameter() = delete;
  using AbstractParameter::AbstractParameter;
};
} // namespace complex
