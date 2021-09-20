#pragma once

#include "complex/Common/Types.hpp"

#include <string>
#include <string_view>

namespace complex
{
/**
 * @brief BasicStringLiteral is meant to be a safe container for a string literal allowing for easy access to its size/length.
 * This class should always contain a pointer to a static compile time null-terminated string literal.
 * Typical usage will be for static string constants. At this time the constructors allow non string literals to be passed in.
 * This is undesired behavior, but there is no way to overcome this in C++17 without using character parameter packs which come with their own issues.
 * @tparam T Character type
 */
template <class T>
class BasicStringLiteral
{
public:
  BasicStringLiteral() = delete;

  /**
   * @brief Constructor that accepts a string literal of fixed size.
   * @param string
   * @return
   */
  template <usize Size>
  constexpr BasicStringLiteral(const T (&string)[Size]) noexcept
  : m_Size(Size)
  , m_String(string)
  {
  }

  ~BasicStringLiteral() noexcept = default;

  BasicStringLiteral(const BasicStringLiteral&) noexcept = default;
  BasicStringLiteral(BasicStringLiteral&&) noexcept = default;

  BasicStringLiteral& operator=(const BasicStringLiteral&) noexcept = default;
  BasicStringLiteral& operator=(BasicStringLiteral&&) noexcept = default;

  /**
   * @brief Returns the c-string pointer
   * @return const T*
   */
  constexpr const T* c_str() const noexcept
  {
    return m_String;
  }

  /**
   * @brief Returns the size of the string literal including the null terminator
   * @return usize
   */
  constexpr usize size() const noexcept
  {
    return m_Size;
  }

  /**
   * @brief Returns the size of the string literal not including the null terminator
   * @return usize
   */
  constexpr usize length() const noexcept
  {
    return m_Size - 1;
  }

  /**
   * @brief Returns a view of string literal not including the null terminator
   * @return std::basic_string_view<T>
   */
  constexpr std::basic_string_view<T> view() const
  {
    return std::basic_string_view<T>(m_String, length());
  }

  /**
   * @brief Returns a view of string literal including the null terminator
   * @return std::basic_string_view<T>
   */
  constexpr std::basic_string_view<T> c_view() const
  {
    return std::basic_string_view<T>(m_String, size());
  }

  /**
   * @brief Returns a null-terminated heap allocated string
   * @return std::basic_string<T>
   */
  std::basic_string<T> str() const
  {
    return std::basic_string<T>(m_String, length());
  }

private:
  const T* m_String;
  usize m_Size;
};

using StringLiteral = BasicStringLiteral<char>;
using WStringLiteral = BasicStringLiteral<wchar_t>;
using String16Literal = BasicStringLiteral<char16_t>;
using String32Literal = BasicStringLiteral<char32_t>;
} // namespace complex
