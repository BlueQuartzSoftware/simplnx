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
  /**
   * @brief Default constructor.
   */
  AnyCloneable() = default;

  /**
   * @brief Constructs from a unique_ptr to T.
   * @param value The unique pointer to wrap
   */
  AnyCloneable(std::unique_ptr<T> value) noexcept
  : m_Value(std::move(value))
  {
  }

  /**
   * @brief Copy constructor clones the wrapped value.
   * @param rhs The AnyCloneable to copy
   */
  AnyCloneable(const AnyCloneable& rhs)
  : m_Value(rhs.m_Value->clone())
  {
  }

  /**
   * @brief Move constructor.
   * @param rhs The AnyCloneable to move
   */
  AnyCloneable(AnyCloneable&& rhs) noexcept = default;

  /**
   * @brief Copy assignment operator clones the wrapped value.
   * @param rhs The AnyCloneable to copy
   * @return AnyCloneable& Reference to this object
   */
  AnyCloneable& operator=(const AnyCloneable& rhs)
  {
    m_Value = rhs.m_Value->clone();
    return *this;
  }

  /**
   * @brief Move assignment operator.
   * @param rhs The AnyCloneable to move
   * @return AnyCloneable& Reference to this object
   */
  AnyCloneable& operator=(AnyCloneable&& rhs) noexcept = default;

  /**
   * @brief Default destructor.
   */
  ~AnyCloneable() noexcept = default;

  /**
   * @brief Returns a pointer to the wrapped value.
   * @return T* Pointer to the wrapped value
   */
  T* get() noexcept
  {
    return m_Value.get();
  }

  /**
   * @brief Returns a const pointer to the wrapped value.
   * @return const T* Const pointer to the wrapped value
   */
  const T* get() const noexcept
  {
    return m_Value.get();
  }

  /**
   * @brief Arrow operator for accessing the wrapped value.
   * @return T* Pointer to the wrapped value
   */
  T* operator->() noexcept
  {
    return m_Value.get();
  }

  /**
   * @brief Const arrow operator for accessing the wrapped value.
   * @return const T* Const pointer to the wrapped value
   */
  const T* operator->() const noexcept
  {
    return m_Value.get();
  }

  /**
   * @brief Returns a reference to the wrapped value. Throws if value is null.
   * @return T& Reference to the wrapped value
   */
  T& getRef()
  {
    if(m_Value == nullptr)
    {
      throw std::runtime_error("AnyCloneable: Null value");
    }
    return *m_Value;
  }

  /**
   * @brief Returns a const reference to the wrapped value. Throws if value is null.
   * @return const T& Const reference to the wrapped value
   */
  const T& getRef() const
  {
    if(m_Value == nullptr)
    {
      throw std::runtime_error("AnyCloneable: Null value");
    }
    return *m_Value;
  }

  /**
   * @brief Returns true if the wrapped value is null.
   * @return bool True if the value is null
   */
  bool isEmpty() const noexcept
  {
    return m_Value == nullptr;
  }

  /**
   * @brief Releases ownership of the wrapped value.
   * @return std::unique_ptr<T> The released unique pointer
   */
  std::unique_ptr<T> release() noexcept
  {
    return std::exchange(m_Value, nullptr);
  }

private:
  std::unique_ptr<T> m_Value;
};
} // namespace nx::core
