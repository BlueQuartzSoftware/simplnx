#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Filter/Arguments.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <nlohmann/json.hpp>

#include <string_view>

namespace nx::core::SIMPLConversion
{
//------------------------------------------------------------------------------
template <class ConverterT>
Result<> ConvertParameter(Arguments& args, const nlohmann::json& json, std::string_view simplKey, const std::string& complexKey)
{
  if(json.contains(simplKey))
  {
    auto result = ConverterT::convert(json[simplKey]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> Convert2Parameters(Arguments& args, const nlohmann::json& json, std::string_view simplKey1, std::string_view simplKey2, const std::string& complexKey)
{
  if(json.contains(simplKey1) && json.contains(simplKey2))
  {
    auto result = ConverterT::convert(json[simplKey1], json[simplKey2]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> Convert3Parameters(Arguments& args, const nlohmann::json& json, std::string_view simplKey1, std::string_view simplKey2, std::string_view simplKey3, const std::string& complexKey)
{
  if(json.contains(simplKey1) && json.contains(simplKey2) && json.contains(simplKey3))
  {
    auto result = ConverterT::convert(json[simplKey1], json[simplKey2], json[simplKey3]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> ConvertTopParameters(Arguments& args, const nlohmann::json& json, const std::string& complexKey)
{
  {
    auto result = ConverterT::convert(json);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<std::string> ReadDataContainerName(const nlohmann::json& json, std::string_view parameterName);

//------------------------------------------------------------------------------
Result<std::string> ReadAttributeMatrixName(const nlohmann::json& json, std::string_view parameterName);

//------------------------------------------------------------------------------
Result<std::string> ReadDataArrayName(const nlohmann::json& json, std::string_view parameterName);

//------------------------------------------------------------------------------
Result<ChoicesParameter::ValueType> ConvertChoicesParameter(const nlohmann::json& json, std::string_view parameterName);

//------------------------------------------------------------------------------
Result<FileSystemPathParameter::ValueType> ReadInputFilePath(const nlohmann::json& json, std::string_view parameterName);
} // namespace nx::core::SIMPLConversion
