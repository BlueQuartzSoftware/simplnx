#pragma once

#include "complex/Filter/AbstractParameter.hpp"

namespace complex
{
class COMPLEX_EXPORT ValueParameter : public AbstractParameter
{
public:
  ~ValueParameter() noexcept override = default;

  ValueParameter(const ValueParameter&) = delete;
  ValueParameter(ValueParameter&&) noexcept = delete;

  ValueParameter& operator=(const ValueParameter&) = delete;
  ValueParameter& operator=(ValueParameter&&) noexcept = delete;

  [[nodiscard]] Type type() const final;

  [[nodiscard]] virtual bool validate(const std::any& value) const = 0;

protected:
  ValueParameter() = delete;
  using AbstractParameter::AbstractParameter;
};
} // namespace complex
