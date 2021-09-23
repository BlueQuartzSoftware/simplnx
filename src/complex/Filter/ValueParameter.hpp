#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Filter/AbstractParameter.hpp"

namespace complex
{
/**
 * @brief Superclass for all Value type of FilterParameters
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
   * @brief Returns the type of parameter. Value Type for this class
   * @return
   */
  [[nodiscard]] Type type() const final;

  /**
   * @brief This function will check if the supplied value passes the validation
   * rules of the FilterParameter. This should be overridden in subclasses.
   * @param value The value to check
   * @return  The results of the validation
   */
  [[nodiscard]] virtual Result<> validate(const std::any& value) const = 0;

protected:
  ValueParameter() = delete;
  using AbstractParameter::AbstractParameter;
};
} // namespace complex
