#include "CropGeometryParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
namespace
{
constexpr StringLiteralType k_Type_Key = "type";
constexpr StringLiteralType k_Is2D_Key = "is_2d";
constexpr StringLiteralType k_CropX_Key = "crop_x";
constexpr StringLiteralType k_CropY_Key = "crop_y";
constexpr StringLiteralType k_CropZ_Key = "crop_z";
constexpr StringLiteralType k_XBoundsVoxels_Key = "bounds_x_voxels";
constexpr StringLiteralType k_YBoundsVoxels_Key = "bounds_y_voxels";
constexpr StringLiteralType k_ZBoundsVoxels_Key = "bounds_z_voxels";
constexpr StringLiteralType k_XBoundsPhysical_Key = "bounds_x_physical";
constexpr StringLiteralType k_YBoundsPhysical_Key = "bounds_y_physical";
constexpr StringLiteralType k_ZBoundsPhysical_Key = "bounds_z_physical";

constexpr int64 k_InvalidType = -63900;
constexpr int64 k_InvalidBounds = -63901;
} // namespace

CropGeometryParameter::CropGeometryParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
{
}

Uuid CropGeometryParameter::uuid() const
{
  return ParameterTraits<CropGeometryParameter>::uuid;
}

IParameter::AcceptedTypesType CropGeometryParameter::acceptedTypes() const
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
  json[k_Is2D_Key] = cropValues.is_2d;
  json[k_CropX_Key] = cropValues.crop_x;
  json[k_CropY_Key] = cropValues.crop_y;
  json[k_CropZ_Key] = cropValues.crop_z;
  json[k_XBoundsVoxels_Key] = cropValues.x_bound_voxels;
  json[k_YBoundsVoxels_Key] = cropValues.y_bound_voxels;
  json[k_ZBoundsVoxels_Key] = cropValues.z_bound_voxels;
  json[k_XBoundsPhysical_Key] = cropValues.x_bound_physical;
  json[k_YBoundsPhysical_Key] = cropValues.y_bound_physical;
  json[k_ZBoundsPhysical_Key] = cropValues.z_bound_physical;
  return json;
}

Result<std::any> CropGeometryParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'CropGeometryParameter' JSON Error: ";
  static const std::string nameDiv = name() + "/";

  ValueType value;
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not an object", k_Prefix.view(), name()));
  }

  const auto& keyJson = json[k_Type_Key];
  if(!keyJson.is_number_unsigned())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                     fmt::format("{}JSON value for key '{}' is not an unsigned integer", k_Prefix.view(), nameDiv + k_Type_Key.str()));
  }
  value.type = static_cast<ValueType::TypeEnum>(keyJson.get<uint8>());

  if(json.contains(k_Is2D_Key))
  {
    const auto& is2dJson = json[k_Is2D_Key];
    if(!is2dJson.is_boolean())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a boolean", k_Prefix.view(), nameDiv + k_Is2D_Key.str()));
    }
    value.is_2d = is2dJson.get<bool>();
  }

  {
    const auto& cropXJson = json[k_CropX_Key];
    if(!cropXJson.is_boolean())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a boolean", k_Prefix.view(), nameDiv + k_CropX_Key.str()));
    }
    value.crop_x = cropXJson.get<bool>();
  }

  {
    const auto& cropYJson = json[k_CropY_Key];
    if(!cropYJson.is_boolean())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a boolean", k_Prefix.view(), nameDiv + k_CropY_Key.str()));
    }
    value.crop_y = cropYJson.get<bool>();
  }

  {
    const auto& cropZJson = json[k_CropZ_Key];
    if(!cropZJson.is_boolean())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a boolean", k_Prefix.view(), nameDiv + k_CropZ_Key.str()));
    }
    value.crop_z = cropZJson.get<bool>();
  }

  {
    const auto& xBoundsJson = json[k_XBoundsVoxels_Key];
    if(!xBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_XBoundsVoxels_Key.str()));
    }
    const auto tmp = xBoundsJson.get<std::array<usize, 2>>();
    value.x_bound_voxels = {tmp[0], tmp[1]};
  }

  {
    const auto& yBoundsJson = json[k_YBoundsVoxels_Key];
    if(!yBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_YBoundsVoxels_Key.str()));
    }
    const auto tmp = yBoundsJson.get<std::array<usize, 2>>();
    value.y_bound_voxels = {tmp[0], tmp[1]};
  }

  {
    const auto& zBoundsJson = json[k_ZBoundsVoxels_Key];
    if(!zBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_ZBoundsVoxels_Key.str()));
    }
    const auto tmp = zBoundsJson.get<std::array<usize, 2>>();
    value.z_bound_voxels = {tmp[0], tmp[1]};
  }

  {
    const auto& xBoundsJson = json[k_XBoundsPhysical_Key];
    if(!xBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_XBoundsPhysical_Key.str()));
    }
    const auto tmp = xBoundsJson.get<std::array<float32, 2>>();
    value.x_bound_physical = {static_cast<float32>(tmp[0]), static_cast<float32>(tmp[1])};
  }

  {
    const auto& yBoundsJson = json[k_YBoundsPhysical_Key];
    if(!yBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_YBoundsPhysical_Key.str()));
    }
    const auto tmp = yBoundsJson.get<std::array<float32, 2>>();
    value.y_bound_physical = {static_cast<float32>(tmp[0]), static_cast<float32>(tmp[1])};
  }

  {
    const auto& zBoundsJson = json[k_ZBoundsPhysical_Key];
    if(!zBoundsJson.is_array())
    {
      return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString,
                                       fmt::format("{}JSON value for key '{}' is not an array", k_Prefix.view(), nameDiv + k_ZBoundsPhysical_Key.str()));
    }
    const auto tmp = zBoundsJson.get<std::array<float32, 2>>();
    value.z_bound_physical = {static_cast<float32>(tmp[0]), static_cast<float32>(tmp[1])};
  }

  return {value};
}

IParameter::UniquePointerType CropGeometryParameter::clone() const
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

  if(cropValues.type == CropValues::TypeEnum::VoxelSubvolume)
  {
    if(cropValues.x_bound_voxels[0] > cropValues.x_bound_voxels[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid X bounds: min ({}) is greater than max ({})", cropValues.x_bound_voxels[0], cropValues.x_bound_voxels[1]));
    }
    if(cropValues.y_bound_voxels[0] > cropValues.y_bound_voxels[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid Y bounds: min ({}) is greater than max ({})", cropValues.y_bound_voxels[0], cropValues.y_bound_voxels[1]));
    }
    if(cropValues.z_bound_voxels[0] > cropValues.z_bound_voxels[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid Z bounds: min ({}) is greater than max ({})", cropValues.z_bound_voxels[0], cropValues.z_bound_voxels[1]));
    }
  }
  else if(cropValues.type == CropValues::TypeEnum::PhysicalSubvolume)
  {
    if(cropValues.x_bound_physical[0] > cropValues.x_bound_physical[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid X bounds: min ({}) is greater than max ({})", cropValues.x_bound_physical[0], cropValues.x_bound_physical[1]));
    }
    if(cropValues.y_bound_physical[0] > cropValues.y_bound_physical[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid Y bounds: min ({}) is greater than max ({})", cropValues.y_bound_physical[0], cropValues.y_bound_physical[1]));
    }
    if(cropValues.z_bound_physical[0] > cropValues.z_bound_physical[1])
    {
      return MakeErrorResult(k_InvalidBounds, fmt::format("Invalid Z bounds: min ({}) is greater than max ({})", cropValues.z_bound_physical[0], cropValues.z_bound_physical[1]));
    }
  }

  return {};
}
} // namespace nx::core
