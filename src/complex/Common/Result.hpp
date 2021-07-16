#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <nonstd/expected.hpp>

#include "complex/Common/Types.hpp"

namespace complex
{
struct Error
{
  i32 code = 0;
  std::string message;
};

struct Warning
{
  i32 code = 0;
  std::string message;
};

namespace detail
{
template <class T>
struct ResultBase
{
  [[nodiscard]] T& value()
  {
    return m_Expected.value();
  }

  [[nodiscard]] const T& value() const
  {
    return m_Expected.value();
  }

  nonstd::expected<T, std::vector<Error>> m_Expected;
};

struct ResultVoid
{
  nonstd::expected<void, std::vector<Error>> m_Expected;
};
} // namespace detail

template <class T = void>
struct Result : public std::conditional_t<std::is_same_v<T, void>, detail::ResultVoid, detail::ResultBase<T>>
{
  [[nodiscard]] bool valid() const
  {
    return m_Expected.has_value();
  }

  [[nodiscard]] std::vector<Error>& errors()
  {
    return m_Expected.error();
  }

  [[nodiscard]] const std::vector<Error>& errors() const
  {
    return m_Expected.error();
  }

  [[nodiscard]] std::vector<Warning>& warnings()
  {
    return m_Warnings;
  }

  [[nodiscard]] const std::vector<Warning>& warnings() const
  {
    return m_Warnings;
  }

  std::vector<Warning> m_Warnings;
};
} // namespace complex
