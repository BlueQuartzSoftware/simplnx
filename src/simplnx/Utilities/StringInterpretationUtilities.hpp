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
std::string TypeToFuncName()
{
  if constexpr(std::is_floating_point_v<T>)
  {
    if constexpr(std::is_same_v<T, float32>)
    {
      return "std::stof";
    }
    if constexpr(std::is_same_v<T, float64>)
    {
      return "std::stod";
    }
  }
  if constexpr(std::is_unsigned_v<T>)
  {
    return "std::stoull";
  }

  // is signed and not float
  return "std::stoll";
}

template <typename T>
Result<T> StringInterpreterFromType(const std::string& input)
{
  T outputValue;

  // This is segmented out to contain the need for apple specific macros in the case of error
  std::string typeName = "usize";
#ifdef __APPLE__
  if constexpr(!std::is_same_v<T, usize>)
  {
    typeName = DataTypeToString(GetDataType<T>());
  }
#else
  typeName = DataTypeToString(GetDataType<T>());
#endif

  try
  {
    if constexpr(std::is_floating_point_v<T>)
    {
      if constexpr(std::is_same_v<T, float32>)
      {
        float32 value = std::stof(input);
        outputValue = static_cast<T>(value);
      }
      else if constexpr(std::is_same_v<T, float64>)
      {
        float64 value = std::stod(input);
        outputValue = static_cast<T>(value);
      }
    }
    else
    {
      if constexpr(std::is_unsigned_v<T>)
      {
        if(!input.empty() && input.at(0) == '-')
        {
          return nx::core::MakeErrorResult<T>(-10350, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, typeName, detail::TypeToFuncName<T>()));
        }

        uint64 value = std::stoull(input);
        if(value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
          return nx::core::MakeErrorResult<T>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, typeName, detail::TypeToFuncName<T>()));
        }
        outputValue = static_cast<T>(value);
      }
      else
      {
        // Default: is signed and not float
        int64 value = std::stoll(input);
        if(value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
          return nx::core::MakeErrorResult<T>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, typeName, detail::TypeToFuncName<T>()));
        }
        outputValue = static_cast<T>(value);
      }
    }
  } catch(const std::invalid_argument& e)
  {
    return nx::core::MakeErrorResult<T>(-10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, typeName, detail::TypeToFuncName<T>()));
  } catch(const std::out_of_range& e)
  {
    return nx::core::MakeErrorResult<T>(-10352, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, typeName, detail::TypeToFuncName<T>()));
  }
  return {outputValue};
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
      Result<int64> result = detail::StringInterpreterFromType<int64>(input);
      if(result.valid())
      {
        return {result.value() != 0};
      }
    }

    {
      Result<uint64> result = detail::StringInterpreterFromType<uint64>(input);
      if(result.valid())
      {
        return {result.value() != 0};
      }
    }

    {
      Result<float64> result = detail::StringInterpreterFromType<float64>(input);
      if(result.valid())
      {
        return {result.value() != 0.0};
      }
    }

    {
      Result<float32> result = detail::StringInterpreterFromType<float32>(input);
      if(result.valid())
      {
        return {result.value() != 0.0};
      }
    }

    return {true};
  }
  else
  {
    return detail::StringInterpreterFromType<T>(input);
  }
}

SIMPLNX_EXPORT Result<> CheckValueConverts(DataType type, const std::string& value);
} // namespace nx::core::StringInterpretationUtilities
