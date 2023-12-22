#pragma once

#include <any>

namespace nx::core
{
/**
 * @brief Returns a reference to the contained value in the std::any.
 * @tparam T
 * @param value
 * @return T&
 */
template <class T>
T& GetAnyRef(std::any& value)
{
  auto* valuePtr = std::any_cast<T>(&value);
  if(valuePtr == nullptr)
  {
    throw std::bad_any_cast();
  }
  return *valuePtr;
}

/**
 * @brief Returns a reference to the contained value in the std::any.
 * @tparam T
 * @param value
 * @return const T&
 */
template <class T>
const T& GetAnyRef(const std::any& value)
{
  const auto* valuePtr = std::any_cast<T>(&value);
  if(valuePtr == nullptr)
  {
    throw std::bad_any_cast();
  }
  return *valuePtr;
}
} // namespace nx::core
