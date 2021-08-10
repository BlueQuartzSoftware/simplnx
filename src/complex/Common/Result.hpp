#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <nonstd/expected.hpp>

#include "complex/Common/Types.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
struct COMPLEX_EXPORT Error
{
  i32 code = 0;
  std::string message;
};

struct COMPLEX_EXPORT Warning
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

struct COMPLEX_EXPORT ResultVoid
{
  nonstd::expected<void, std::vector<Error>> m_Expected;
};

template <class T>
using ResultBaseT = std::conditional_t<std::is_same_v<T, void>, detail::ResultVoid, detail::ResultBase<T>>;
} // namespace detail

template <class T = void>
struct Result : public detail::ResultBaseT<T>
{
  [[nodiscard]] bool valid() const
  {
    return detail::ResultBaseT<T>::m_Expected.has_value();
  }

  [[nodiscard]] std::vector<Error>& errors()
  {
    return detail::ResultBaseT<T>::m_Expected.error();
  }

  [[nodiscard]] const std::vector<Error>& errors() const
  {
    return detail::ResultBaseT<T>::m_Expected.error();
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

template <class T>
Result<> convertResult(Result<T>&& result)
{
  Result<> voidResult;
  if(!result.valid())
  {
    voidResult.m_Expected = nonstd::make_unexpected(std::move(result.errors()));
  }
  voidResult.warnings() = std::move(result.warnings());
  return voidResult;
}
} // namespace complex
