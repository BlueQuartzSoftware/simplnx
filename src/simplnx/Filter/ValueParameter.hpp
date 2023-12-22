#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Filter/AbstractParameter.hpp"

namespace nx::core
{
/**
 * @brief ValueParameter provides an interface for parameters that deal with value types (e.g. integer, string, etc.).
 */
class SIMPLNX_EXPORT ValueParameter : public AbstractParameter
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
  Type type() const final;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return
   */
  virtual Result<> validate(const std::any& value) const = 0;

protected:
  ValueParameter() = delete;
  using AbstractParameter::AbstractParameter;
};
} // namespace nx::core
