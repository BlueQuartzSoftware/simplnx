#pragma once

#include "complex/Filter/IParameter.hpp"

namespace complex
{
/**
 * @class AnyParameter
 * @brief AnyParameter is a wrapper with value semantics around IParameter.
 */
class AnyParameter
{
public:
  AnyParameter() = default;

  AnyParameter(IParameter::UniquePointer parameter) noexcept
  : m_Parameter(std::move(parameter))
  {
  }

  AnyParameter(const AnyParameter& rhs)
  : m_Parameter(rhs.m_Parameter->clone())
  {
  }

  AnyParameter(AnyParameter&& rhs) noexcept = default;

  AnyParameter& operator=(const AnyParameter& rhs)
  {
    m_Parameter = rhs.m_Parameter->clone();
    return *this;
  }

  AnyParameter& operator=(AnyParameter&& rhs) noexcept = default;

  ~AnyParameter() noexcept = default;

  IParameter* get() noexcept
  {
    return m_Parameter.get();
  }

  const IParameter* get() const noexcept
  {
    return m_Parameter.get();
  }

  IParameter* operator->() noexcept
  {
    return m_Parameter.get();
  }

  const IParameter* operator->() const noexcept
  {
    return m_Parameter.get();
  }

  bool isEmpty() const noexcept
  {
    return m_Parameter == nullptr;
  }

  IParameter::UniquePointer release() noexcept
  {
    return std::exchange(m_Parameter, nullptr);
  }

private:
  IParameter::UniquePointer m_Parameter;
};
} // namespace complex
