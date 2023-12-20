#pragma once

#include "simplnx/Filter/DataParameter.hpp"

namespace nx::core
{
/**
 * @brief ConstDataParameter provides const access to DataStructure for parameters.
 */
class SIMPLNX_EXPORT ConstDataParameter : public DataParameter
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
  Mutability mutability() const final;

  /**
   * @brief Takes the value and a const DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  virtual Result<std::any> resolve(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  ConstDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace nx::core
