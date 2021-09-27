#pragma once

#include "complex/Filter/DataParameter.hpp"

namespace complex
{
/**
 * @brief MutableDataParameter provides const access to DataStructure for parameters.
 */
class COMPLEX_EXPORT ConstDataParameter : public DataParameter
{
public:
  ~ConstDataParameter() noexcept override = default;

  ConstDataParameter(const ConstDataParameter&) = delete;
  ConstDataParameter(ConstDataParameter&&) noexcept = delete;

  ConstDataParameter& operator=(const ConstDataParameter&) = delete;
  ConstDataParameter& operator=(ConstDataParameter&&) noexcept = delete;

  /**
   * @brief Returns whether the parameter needs const or non-const access to the DataStructure.
   * @return
   */
  [[nodiscard]] Mutability mutability() const final;

  /**
   * @brief Takes the value and a const DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  [[nodiscard]] virtual Result<std::any> resolve(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  ConstDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace complex
