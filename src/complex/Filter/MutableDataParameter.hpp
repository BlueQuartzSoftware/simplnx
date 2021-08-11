#pragma once

#include "complex/Filter/DataParameter.hpp"

namespace complex
{
class COMPLEX_EXPORT MutableDataParameter : public DataParameter
{
public:
  ~MutableDataParameter() noexcept override = default;

  MutableDataParameter(const MutableDataParameter&) = delete;
  MutableDataParameter(MutableDataParameter&&) noexcept = delete;

  MutableDataParameter& operator=(const MutableDataParameter&) = delete;
  MutableDataParameter& operator=(MutableDataParameter&&) noexcept = delete;

  [[nodiscard]] Mutability mutability() const final;

  [[nodiscard]] virtual Result<std::any> resolve(DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  MutableDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace complex
