#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <nonstd/expected.hpp>

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Error is a simple struct of a error code and message.
 * It is meant for general purpose error reporting.
 */
struct COMPLEX_EXPORT Error
{
  int32 code = 0;
  std::string message;
};

/**
 * @brief Warning is a simple struct of a warning code and message.
 * It is meant for general purpose warning reporting.
 */
struct COMPLEX_EXPORT Warning
{
  int32 code = 0;
  std::string message;
};

namespace detail
{
template <class T>
struct ResultBase
{
  /**
   * @brief Returns a reference to the contained value if valid. Otherwise throws.
   * @return T&
   */
  T& value()
  {
    return m_Expected.value();
  }

  /**
   * @brief Returns a const reference to the contained value if valid. Otherwise throws.
   * @return const T&
   */
  const T& value() const
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
  bool valid() const
  {
    return detail::ResultBaseT<T>::m_Expected.has_value();
  }

  /**
   * @brief Equivalent to !valid()
   * @return
   */
  bool invalid() const
  {
    return !valid();
  }

  /**
   * @brief Returns the collection of Error objects
   * @return
   */
  std::vector<Error>& errors()
  {
    return detail::ResultBaseT<T>::m_Expected.error();
  }

  /**
   * @brief Returns the collection of Error objects
   * @return
   */
  const std::vector<Error>& errors() const
  {
    return detail::ResultBaseT<T>::m_Expected.error();
  }

  /**
   * @brief Returns the collection of Warning Objects
   * @return
   */
  std::vector<Warning>& warnings()
  {
    return m_Warnings;
  }

  /**
   * @brief Returns the collection of Warning Objects
   * @return
   */
  const std::vector<Warning>& warnings() const
  {
    return m_Warnings;
  }

  std::vector<Warning> m_Warnings;
};

/**
 * @brief Converts a Result<T> to a Result<> moving the errors and warnings while discarding the value.
 * @tparam T
 * @param result
 * @return Result<>
 */
template <class T>
Result<> ConvertResult(Result<T>&& result)
{
  Result<> voidResult;
  if(!result.valid())
  {
    voidResult.m_Expected = nonstd::make_unexpected(std::move(result.errors()));
  }
  voidResult.warnings() = std::move(result.warnings());
  return voidResult;
}

template <class T>
Result<T> CovertResultTo(Result<>&& fromResult, T&& value)
{
  Result<T> convertedResult;
  if(fromResult.valid())
  {
    convertedResult = {std::move(value)};
  }
  else
  {
    // Plase errors from 'result' into outputActions
    convertedResult.m_Expected = nonstd::make_unexpected(std::move(fromResult.errors()));
  }
  // Always move the warnings AFTER the other check above...
  convertedResult.warnings() = std::move(fromResult.warnings());

  return convertedResult;
}

/**
 * @brief Convenience function to generate a standard error Result Object
 *
 * A Typical invocation would be:
 * @code
 *     return complex::MakeErrorResult<std::any>(-1, fmt::format("{} JSON Data does not contain an entry with a key of \"{}\"", prefix, name()));
 * @endcode
 * @tparam T
 * @param code Error Value. Typically negative number
 * @param message The Error message that is paired with the code
 * @return Result Object
 */
template <class T = void>
Result<T> MakeErrorResult(int32 code, std::string message)
{
  return {nonstd::make_unexpected(std::vector<Error>{{code, std::move(message)}})};
}

/**
 * @brief Generates a Result<> with a singular warning.
 * @param code
 * @param message
 * @return
 */
inline Result<> MakeWarningVoidResult(int32 code, std::string message)
{
  Result<> result;
  result.warnings().push_back(Warning{code, std::move(message)});
  return result;
}
} // namespace complex
