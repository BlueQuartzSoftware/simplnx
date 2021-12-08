#pragma once

#include "complex/Filter/IParameter.hpp"

#include <stdexcept>

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

  IParameter& getRef()
  {
    if(m_Parameter == nullptr)
    {
      throw std::runtime_error("AnyParameter: Null IParameter");
    }
    return *m_Parameter;
  }

  const IParameter& getRef() const
  {
    if(m_Parameter == nullptr)
    {
      throw std::runtime_error("AnyParameter: Null IParameter");
    }
    return *m_Parameter;
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
