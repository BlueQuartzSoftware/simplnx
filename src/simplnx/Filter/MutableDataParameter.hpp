#pragma once

#include "simplnx/Filter/DataParameter.hpp"

namespace nx::core
{
/**
 * @brief MutableDataParameter provides mutable access to DataStructure for parameters.
 */
class SIMPLNX_EXPORT MutableDataParameter : public DataParameter
{
public:
  ~MutableDataParameter() noexcept override = default;

  MutableDataParameter(const MutableDataParameter&) = delete;
  MutableDataParameter(MutableDataParameter&&) noexcept = delete;

  MutableDataParameter& operator=(const MutableDataParameter&) = delete;
  MutableDataParameter& operator=(MutableDataParameter&&) noexcept = delete;

  /**
   * @brief Returns whether the parameter needs const or non-const access to the DataStructure.
   * @return
   */
  Mutability mutability() const final;

  /**
   * @brief Takes the value and a mutable DataStructure and attempts store the actual derived DataObject in the std::any.
   * Returns any warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  virtual Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  MutableDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace nx::core
