#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"

#include <fmt/format.h>

#include <string>

namespace nx::core::StringInterpretationUtilities
{
namespace detail
{
template <typename T>
std::function<Result<T>(const std::string&)> StringInterpreterFromType()
{
  if constexpr(std::is_floating_point_v<T>)
  {
    if constexpr(std::is_same_v<T, float32>)
    {
      return [](const std::string& input) -> Result<T> {
        try
        {
          float32 value = std::stof(input);
          return {static_cast<T>(value)};
        } catch(const std::invalid_argument& e)
        {
          return nx::core::MakeErrorResult<T>(
              -10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()), typeid(detail::StringInterpreterFromType<T>()).name()));
        } catch(const std::out_of_range& e)
        {
          return nx::core::MakeErrorResult<T>(-10352, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                                  typeid(detail::StringInterpreterFromType<T>()).name()));
        }
      };
    }
    if constexpr(std::is_same_v<T, float64>)
    {
      return [](const std::string& input) -> Result<T> {
        try
        {
          float64 value = std::stod(input);
          return {static_cast<T>(value)};
        } catch(const std::invalid_argument& e)
        {
          return nx::core::MakeErrorResult<T>(
              -10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()), typeid(detail::StringInterpreterFromType<T>()).name()));
        } catch(const std::out_of_range& e)
        {
          return nx::core::MakeErrorResult<T>(-10352, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                                  typeid(detail::StringInterpreterFromType<T>()).name()));
        }
      };
    }
  }
  if constexpr(std::is_unsigned_v<T>)
  {
    return [](const std::string& input) -> Result<T> {
      try
      {
        uint64 value = std::stoull(input);
        return {static_cast<T>(value)};
      } catch(const std::invalid_argument& e)
      {
        return nx::core::MakeErrorResult<T>(
            -10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()), typeid(detail::StringInterpreterFromType<T>()).name()));
      } catch(const std::out_of_range& e)
      {
        return nx::core::MakeErrorResult<T>(-10352, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                                typeid(detail::StringInterpreterFromType<T>()).name()));
      }
    };
  }

  // is signed and not float
  return [](const std::string& input) -> Result<T> {
    try
    {
      int64 value = std::stoll(input);
      return {static_cast<T>(value)};
    } catch(const std::invalid_argument& e)
    {
      return nx::core::MakeErrorResult<T>(
          -10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()), typeid(detail::StringInterpreterFromType<T>()).name()));
    } catch(const std::out_of_range& e)
    {
      return nx::core::MakeErrorResult<T>(-10352, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                              typeid(detail::StringInterpreterFromType<T>()).name()));
    }
  };
}
} // namespace detail

template <typename T>
Result<T> Convert(const std::string& input)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    if(input == "TRUE" || input == "true" || input == "True")
    {
      return {true};
    }

    if(input == "FALSE" || input == "false" || input == "False")
    {
      return {false};
    }

    // We are about to run the gauntlet of std::sto functions, if all of them err out true will be returned
    {
      Result<int64> result = detail::StringInterpreterFromType<int64>()(input);
      if(result.valid())
      {
        return {result.value() != 0};
      }
    }

    {
      Result<uint64> result = detail::StringInterpreterFromType<uint64>()(input);
      if(result.valid())
      {
        return {result.value() != 0};
      }
    }

    {
      Result<float64> result = detail::StringInterpreterFromType<float64>()(input);
      if(result.valid())
      {
        return {result.value() != 0.0};
      }
    }

    {
      Result<float32> result = detail::StringInterpreterFromType<float32>()(input);
      if(result.valid())
      {
        return {result.value() != 0.0};
      }
    }

    return {true};
  }

  if constexpr(!std::is_floating_point_v<T> && std::is_unsigned_v<T>)
  {
    if(!input.empty() && input.at(0) == '-')
    {
      return nx::core::MakeErrorResult<T>(-10350, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                              typeid(detail::StringInterpreterFromType<T>()).name()));
    }
  }

  Result<T> result = detail::StringInterpreterFromType<T>()(input);
  if(result.invalid())
  {
    return result;
  }

  if constexpr(std::is_floating_point_v<T>)
  {
    if(result.value() > std::numeric_limits<T>::max() || result.value() < std::numeric_limits<T>::min())
    {
      return nx::core::MakeErrorResult<T>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, DataTypeToString(GetDataType<T>()),
                                                              typeid(detail::StringInterpreterFromType<T>()).name()));
    }
  }

  return result;
}

Result<> CheckValueConverts(DataType type, const std::string& value);
} // namespace nx::core::StringInterpretationUtilities
