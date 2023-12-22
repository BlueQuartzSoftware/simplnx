#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include <nonstd/expected.hpp>

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Error is a simple struct of a error code and message.
 * It is meant for general purpose error reporting.
 */
struct SIMPLNX_EXPORT Error
{
  nx::core::types::int32 code = 0;
  std::string message;
};

using ErrorCollection = std::vector<Error>;

/**
 * @brief Warning is a simple struct of a warning code and message.
 * It is meant for general purpose warning reporting.
 */
struct SIMPLNX_EXPORT Warning
{
  int32 code = 0;
  std::string message;
};

using WarningCollection = std::vector<Warning>;

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

struct SIMPLNX_EXPORT ResultVoid
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
Result<T> ConvertResultTo(Result<>&& fromResult, T&& value)
{
  Result<T> convertedResult;
  if(fromResult.valid())
  {
    convertedResult = {std::forward<T>(value)};
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

template <class ToT, class FromT>
Result<ToT> ConvertInvalidResult(Result<FromT>&& result)
{
  Result<ToT> convertedResult;
  convertedResult.m_Expected = nonstd::make_unexpected(std::move(result.errors()));
  convertedResult.warnings() = std::move(result.warnings());

  return convertedResult;
}

template <class ToT, class FromT>
std::enable_if_t<std::is_convertible_v<FromT, ToT>, Result<ToT>> ConvertResultTo(Result<FromT>&& from)
{
  if(from.invalid())
  {
    return {{nonstd::make_unexpected(std::move(from.errors()))}, std::move(from.warnings())};
  }

  return {{std::move(from.value())}, std::move(from.warnings())};
}

/**
 * @brief Convenience function to generate a standard error Result Object
 *
 * A Typical invocation would be:
 * @code
 *     return nx::core::MakeErrorResult<std::any>(-1, fmt::format("{} JSON Data does not contain an entry with a key of '{}'", prefix, name()));
 * @endcode
 * @tparam T
 * @param code Error Value. Typically negative number
 * @param message The Error message that is paired with the code
 * @return Result Object
 */
template <class T = void>
Result<T> MakeErrorResult(int32 code, std::string message)
{
  return {{nonstd::make_unexpected(std::vector<Error>{{code, std::move(message)}})}};
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

/**
 * @brief Extracts a Result<> into vectors of errors/warnings. Returns the validity of the result.
 * @param result
 * @param errors
 * @param warnings
 * @return
 */
inline bool ExtractResult(Result<> result, std::vector<Error>& errors, std::vector<Warning>& warnings)
{
  warnings.reserve(warnings.size() + result.warnings().size());
  for(auto&& warning : result.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  if(result.invalid())
  {
    errors.reserve(errors.size() + result.errors().size());
    for(auto&& error : result.errors())
    {
      errors.push_back(std::move(error));
    }
  }

  return result.valid();
}

/**
 * @brief Merges two Result<> into one. !!! Does NOT merge the Value in results !!!
 * @param first
 * @param second
 * @return
 */
template <typename T = void>
inline Result<T> MergeResults(Result<T> first, Result<T> second)
{
  usize warningsSize = first.warnings().size() + second.warnings().size();
  std::vector<Warning> warnings;
  warnings.reserve(warningsSize);

  for(auto&& warning : first.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  for(auto&& warning : second.warnings())
  {
    warnings.push_back(std::move(warning));
  }

  usize errorsSize = 0;
  if(first.invalid())
  {
    errorsSize += first.errors().size();
  }
  if(second.invalid())
  {
    errorsSize += second.errors().size();
  }
  std::vector<Error> errors;
  errors.reserve(errorsSize);

  if(first.invalid())
  {
    for(auto&& error : first.errors())
    {
      errors.push_back(std::move(error));
    }
  }
  if(second.invalid())
  {
    for(auto&& error : second.errors())
    {
      errors.push_back(std::move(error));
    }
  }

  Result<T> result = errors.empty() ? Result<T>{} : Result<T>{nonstd::make_unexpected(std::move(errors))};
  result.warnings() = std::move(warnings);
  return result;
}

inline Result<> MergeResults(std::vector<Result<>> results)
{
  Result<> outputResult;

  for(auto& result : results)
  {
    outputResult = MergeResults(std::move(outputResult), std::move(result));
  }

  return outputResult;
}
} // namespace nx::core
