#pragma once

#include "complex/Filter/DataParameter.hpp"

namespace complex
{
class COMPLEX_EXPORT ConstDataParameter : public DataParameter
{
public:
  ~ConstDataParameter() noexcept override = default;

  ConstDataParameter(const ConstDataParameter&) = delete;
  ConstDataParameter(ConstDataParameter&&) noexcept = delete;

  ConstDataParameter& operator=(const ConstDataParameter&) = delete;
  ConstDataParameter& operator=(ConstDataParameter&&) noexcept = delete;

  [[nodiscard]] Mutability mutability() const final;

  [[nodiscard]] virtual std::any resolve(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  ConstDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace complex
