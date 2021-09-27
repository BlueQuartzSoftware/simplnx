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

/**
 * @brief Result is meant for reporting errors/warnings from a function with an optional contained type.
 * Functions similiarly to std::optional. Warnings are always accessible, and either the contained type or errors is accessible at a time.
 * @tparam T Contained type. May be void.
 */
template <class T = void>
struct Result : public detail::ResultBaseT<T>
{
  /**
   * @brief Returns true if there are no errors, i.e. it is valid to access the contained type.
   * @return
   */
  [[nodiscard]] bool valid() const
  {
    return detail::ResultBaseT<T>::m_Expected.has_value();
  }

  /**
   * @brief Equivalent to !valid()
   * @return
   */
  [[nodiscard]] bool invalid() const
  {
    return !valid();
  }

  /**
   * @brief Returns the collection of Error objects
   * @return
   */
  [[nodiscard]] std::vector<Error>& errors()
  {
    return detail::ResultBaseT<T>::m_Expected.error();
  }

  /**
   * @brief Returns the collection of Error objects
   * @return
   */
  [[nodiscard]] const std::vector<Error>& errors() const
  {
    return detail::ResultBaseT<T>::m_Expected.error();
  }

  /**
   * @brief Returns the collection of Warning Objects
   * @return
   */
  [[nodiscard]] std::vector<Warning>& warnings()
  {
    return m_Warnings;
  }

  /**
   * @brief Returns the collection of Warning Objects
   * @return
   */
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

/**
 * @brief Convenience function to generate a standard error Result Object
 *
 * A Typical invocation would be:
 * @code
 *     return complex::MakeErrorResult<std::any>(-1, fmt::format("{} JSON Data does not contain an entry with a key of \"{}\"", prefix, name()));
 * @endcode
 * @tparam T
 * @param errorCode Error Value. Typically negative number
 * @param message The Error message that is paired with the code
 * @return Result Object
 */
template <typename T>
Result<T> MakeErrorResult(int32_t errorCode, const std::string& message)
{
  return {nonstd::make_unexpected(std::vector<Error>{{errorCode, message}})};
}
} // namespace complex
