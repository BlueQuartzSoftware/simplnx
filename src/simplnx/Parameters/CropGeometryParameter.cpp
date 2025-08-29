#include "CropGeometryParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
namespace
{
constexpr StringLiteral k_Type_Key = "type";
constexpr StringLiteral k_CropX_Key = "crop_x";
constexpr StringLiteral k_CropY_Key = "crop_y";
constexpr StringLiteral k_CropZ_Key = "crop_z";
constexpr StringLiteral k_XBounds_Key = "bounds_x";
constexpr StringLiteral k_YBounds_Key = "bounds_y";
constexpr StringLiteral k_ZBounds_Key = "bounds_z";

constexpr int64 k_InvalidType = -63900;
constexpr int64 k_InvalidBounds = -63901;
} // namespace

CropGeometryParameter::CropGeometryParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid CropGeometryParameter::uuid() const
{
  return ParameterTraits<CropGeometryParameter>::uuid;
}

IParameter::AcceptedTypes CropGeometryParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType CropGeometryParameter::getVersion() const
{
  return 1;
}

nlohmann::json CropGeometryParameter::toJsonImpl(const std::any& value) const
{
  const auto& cropValues = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_Type_Key] = static_cast<uint8>(cropValues.type);
  json[k_CropX_Key] = cropValues.cropX;
  json[k_CropY_Key] = cropValues.cropY;
  json[k_CropZ_Key] = cropValues.cropZ;
  json[k_XBounds_Key] = cropValues.xBounds;
  json[k_YBounds_Key] = cropValues.yBounds;
  json[k_ZBounds_Key] = cropValues.zBounds;
  return json;
}

Result<std::any> CropGeometryParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CropGeometryParameter' JSON Error: ";
  static const std::string nameDiv = name() + "/";

  ValueType value;
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not an object", prefix.view(), name()));
  }

  auto keyJson = json[k_Type_Key];
  if(!keyJson.is_number_unsigned())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String,
                                     fmt::format("{}JSON value for key '{}' is not an unsigned integer", prefix.view(), nameDiv + k_Type_Key.str()));
  }
  value.type = static_cast<ValueType::TypeEnum>(keyJson.get<uint8>());

  {
    auto cropXJson = json[k_CropX_Key];
    if(!cropXJson.is_boolean())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a boolean", prefix.view(), nameDiv + k_CropX_Key.str()));
    }
    value.cropX = cropXJson.get<bool>();
  }

  {
    auto cropYJson = json[k_CropY_Key];
    if(!cropYJson.is_boolean())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a boolean", prefix.view(), nameDiv + k_CropY_Key.str()));
    }
    value.cropY = cropYJson.get<bool>();
  }

  {
    auto cropZJson = json[k_CropZ_Key];
    if(!cropZJson.is_boolean())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a boolean", prefix.view(), nameDiv + k_CropZ_Key.str()));
    }
    value.cropZ = cropZJson.get<bool>();
  }

  {
    auto xBoundsJson = json[k_XBounds_Key];
    if(!xBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not an array", prefix.view(), nameDiv + k_XBounds_Key.str()));
    }
    value.xBounds = xBoundsJson.get<std::array<int32, 2>>();
  }

  {
    auto yBoundsJson = json[k_YBounds_Key];
    if(!yBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not an array", prefix.view(), nameDiv + k_YBounds_Key.str()));
    }
    value.yBounds = yBoundsJson.get<std::array<int32, 2>>();
  }

  {
    auto zBoundsJson = json[k_ZBounds_Key];
    if(!zBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not an array", prefix.view(), nameDiv + k_ZBounds_Key.str()));
    }
    value.zBounds = zBoundsJson.get<std::array<int32, 2>>();
  }

  return {value};
}

IParameter::UniquePointer CropGeometryParameter::clone() const
{
  return std::make_unique<CropGeometryParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any CropGeometryParameter::defaultValue() const
{
  return defaultPath();
}

typename CropGeometryParameter::ValueType CropGeometryParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> CropGeometryParameter::validate(const std::any& value) const
{
  const auto& cropValues = GetAnyRef<ValueType>(value);

  const auto cropTypeInt = static_cast<uint8>(cropValues.type);
  if(cropTypeInt >= 3)
  {
    return MakeErrorResult(k_InvalidType, fmt::format("Invalid CropGeometry type: '{}'", cropTypeInt));
  }

  if(cropValues.xBounds[0] > cropValues.xBounds[1])
  {
    return MakeErrorResult(k_InvalidBounds, "Invalid X bounds");
  }
  if(cropValues.yBounds[0] > cropValues.yBounds[1])
  {
    return MakeErrorResult(k_InvalidBounds, "Invalid Y bounds");
  }
  if(cropValues.zBounds[0] > cropValues.zBounds[1])
  {
    return MakeErrorResult(k_InvalidBounds, "Invalid Z bounds");
  }

  return {};
}
} // namespace nx::core
