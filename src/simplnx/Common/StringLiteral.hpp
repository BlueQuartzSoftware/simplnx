#pragma once

#include "simplnx/Common/Types.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace nx::core
{
namespace detail
{
/**
 * @brief Returns true if the given character array is null-terminated.
 * @tparam T
 * @param string
 * @return
 */
template <class T, usize Size>
constexpr bool HasNullTerminator(const T (&string)[Size]) noexcept
{
  return string[Size - 1] == static_cast<T>('\0');
}
} // namespace detail

/**
 * @brief BasicStringLiteral is meant to be a safe container for a string literal allowing for easy access to its size/length.
 * This class should always contain a pointer to a static compile time null-terminated string literal.
 * Typical usage will be for static string constants. At this time the constructors allow non string literals to be passed in.
 * This is undesired behavior, but there is no way to overcome this in C++17 without using character parameter packs which come with their own issues.
 * Example:
 * @code
 * static constexpr StringLiteral k_Foo = "foo";
 * @endcode
 * @tparam T Character type
 */
template <class T>
class BasicStringLiteral
{
public:
  BasicStringLiteral() = delete;

  /**
   * @brief Constructor that accepts a string literal of fixed size. Should be made consteval in C++20.
   * Requires string to be null-terminated.
   * @param string
   * @return
   */
  template <usize Size>
  constexpr BasicStringLiteral(const T (&string)[Size])
  : m_String(string)
  , m_Size(Size)
  {
    if(!detail::HasNullTerminator(string))
    {
      throw std::runtime_error("BasicStringLiteral must be null-terminated");
    }
  }

  ~BasicStringLiteral() noexcept = default;

  BasicStringLiteral(const BasicStringLiteral&) noexcept = default;
  BasicStringLiteral(BasicStringLiteral&&) noexcept = default;

  BasicStringLiteral& operator=(const BasicStringLiteral&) noexcept = default;
  BasicStringLiteral& operator=(BasicStringLiteral&&) noexcept = default;

  /**
   * @brief Returns the c-string pointer.
   * @return const T*
   */
  constexpr const T* c_str() const noexcept
  {
    return m_String;
  }

  /**
   * @brief Returns the size of the string literal not including the null terminator.
   * @return usize
   */
  constexpr usize size() const noexcept
  {
    return m_Size - 1;
  }

  /**
   * @brief Returns a view of string literal not including the null terminator.
   * @return std::basic_string_view<T>
   */
  constexpr std::basic_string_view<T> view() const
  {
    return std::basic_string_view<T>(m_String, size());
  }

  /**
   * @brief Returns a view of string literal including the null terminator.
   * @return std::basic_string_view<T>
   */
  constexpr std::basic_string_view<T> c_view() const
  {
    return std::basic_string_view<T>(m_String, m_Size);
  }

  /**
   * @brief Returns a null-terminated heap allocated string.
   * @return std::basic_string<T>
   */
  std::basic_string<T> str() const
  {
    return std::basic_string<T>(m_String, size());
  }

  /**
   * @brief Implicit conversion to std::basic_string<T>
   */
  operator std::basic_string<T>() const
  {
    return str();
  }

  /**
   * @brief Implicit conversion to std::basic_string_view<T>
   */
  operator std::basic_string_view<T>() const
  {
    return view();
  }

private:
  const T* m_String;
  usize m_Size;
};

template <class T>
bool operator==(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs == rhs.view();
}

template <class T>
bool operator==(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return rhs == lhs;
}

template <class T>
bool operator!=(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs != rhs.view();
}

template <class T>
bool operator!=(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return rhs != lhs;
}

template <class T>
bool operator<(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs < rhs.view();
}

template <class T>
bool operator<(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return lhs.view() < rhs;
}

template <class T>
bool operator>(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs > rhs.view();
}

template <class T>
bool operator>(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return lhs.view() > rhs;
}

template <class T>
bool operator<=(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs <= rhs.view();
}

template <class T>
bool operator<=(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return lhs.view() <= rhs;
}

template <class T>
bool operator>=(const std::basic_string<T>& lhs, BasicStringLiteral<T> rhs)
{
  return lhs >= rhs.view();
}

template <class T>
bool operator>=(BasicStringLiteral<T> lhs, const std::basic_string<T>& rhs)
{
  return lhs.view() >= rhs;
}

using StringLiteral = BasicStringLiteral<char>;
using WStringLiteral = BasicStringLiteral<wchar_t>;
using String16Literal = BasicStringLiteral<char16_t>;
using String32Literal = BasicStringLiteral<char32_t>;
} // namespace nx::core
