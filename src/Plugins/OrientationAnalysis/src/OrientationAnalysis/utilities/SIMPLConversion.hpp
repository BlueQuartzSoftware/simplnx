#pragma once

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include <nlohmann/json.hpp>

#include <string_view>

namespace nx::core::SIMPLConversion
{
struct OEMEbsdScanSelectionFilterParameterConverter
{
  using ParameterType = OEMEbsdScanSelectionParameter;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2)
  {
    static const std::string x = "x";
    static const std::string y = "y";
    static const std::string z = "z";

    if(!json1.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("OEMEbsdScanSelectionParameter json '{}' is not an string", json1.dump()));
    }
    if(!json2.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("OEMEbsdScanSelectionParameter json '{}' is not an array", json2.dump()));
    }

    ValueType value;
    value.inputFilePath = json1.get<std::string>();
    value.scanNames = json2.get<std::list<std::string>>();

    return {std::move(value)};
  }
};
} // namespace nx::core::SIMPLConversion
