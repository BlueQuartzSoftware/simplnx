#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace nx::core
{
/**
 * @class AnyCloneable
 * @brief AnyCloneable is a wrapper with value semantics around any T with a clone() method.
 */
template <class T>
class AnyCloneable
{
  static_assert(std::is_same_v<decltype(std::declval<const T>().clone()), std::unique_ptr<T>>,
                "AnyCloneable only accepts types with a clone method with the following signature: std::unique_ptr<T> clone() const");

public:
  AnyCloneable() = default;

  AnyCloneable(std::unique_ptr<T> value) noexcept
  : m_Value(std::move(value))
  {
  }

  AnyCloneable(const AnyCloneable& rhs)
  : m_Value(rhs.m_Value->clone())
  {
  }

  AnyCloneable(AnyCloneable&& rhs) noexcept = default;

  AnyCloneable& operator=(const AnyCloneable& rhs)
  {
    m_Value = rhs.m_Value->clone();
    return *this;
  }

  AnyCloneable& operator=(AnyCloneable&& rhs) noexcept = default;

  ~AnyCloneable() noexcept = default;

  T* get() noexcept
  {
    return m_Value.get();
  }

  const T* get() const noexcept
  {
    return m_Value.get();
  }

  T* operator->() noexcept
  {
    return m_Value.get();
  }

  const T* operator->() const noexcept
  {
    return m_Value.get();
  }

  T& getRef()
  {
    if(m_Value == nullptr)
    {
      throw std::runtime_error("AnyCloneable: Null value");
    }
    return *m_Value;
  }

  const T& getRef() const
  {
    if(m_Value == nullptr)
    {
      throw std::runtime_error("AnyCloneable: Null value");
    }
    return *m_Value;
  }

  bool isEmpty() const noexcept
  {
    return m_Value == nullptr;
  }

  std::unique_ptr<T> release() noexcept
  {
    return std::exchange(m_Value, nullptr);
  }

private:
  std::unique_ptr<T> m_Value;
};
} // namespace nx::core
