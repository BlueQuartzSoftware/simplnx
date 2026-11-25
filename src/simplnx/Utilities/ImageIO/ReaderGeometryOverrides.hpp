#pragma once

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace nx::core::ImageIO
{
inline constexpr StringLiteral k_ChangeOrigin = "change_origin";
inline constexpr StringLiteral k_Origin = "origin";
inline constexpr StringLiteral k_ChangeSpacing = "change_spacing";
inline constexpr StringLiteral k_Spacing = "spacing";
inline constexpr StringLiteral k_SetOriginLabel = "Set Origin";
inline constexpr StringLiteral k_SetSpacingLabel = "Set Spacing";

/**
 * @brief Formats a stored geometry value error with the settings that replace it.
 * @param filePath Path to the input file.
 * @param field Header field or geometry value that failed validation.
 * @param detail Diagnostic with the offending value.
 * @param setting Label of the setting that replaces the value.
 * @param additionalSetting Label of a second setting if it also uses the value.
 * @return A diagnostic that names the input file and the settings to enable.
 */
[[nodiscard]] inline std::string FormatReaderValueError(const std::filesystem::path& filePath, std::string_view field, std::string_view detail, std::string_view setting,
                                                        std::string_view additionalSetting = {})
{
  const auto settings = additionalSetting.empty() ? fmt::format("'{}'", setting) : fmt::format("'{}' and '{}'", setting, additionalSetting);
  return fmt::format("Input file '{}' has invalid {} values. Error Message: {} Enable {} to use your own values.", filePath.string(), field, detail, settings);
}

/**
 * @brief Adds the linked Set Origin and Set Spacing parameters.
 * @param params Destination parameter list.
 */
inline void InsertGeometryOverrideParameters(Parameters& params)
{
  using V = VectorFloat32Parameter::ValueType;
  using Names = VectorFloat32Parameter::NamesType;
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin, k_SetOriginLabel, "Use the supplied origin for the imported image.", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin, "Origin (Physical Units)", "Origin coordinates in X, Y, Z order.", V{0.0F, 0.0F, 0.0F}, Names{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing, k_SetSpacingLabel, "Use the supplied spacing for the imported image.", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing, "Spacing (Physical Units)", "Positive spacing in X, Y, Z order.", V{1.0F, 1.0F, 1.0F}, Names{"X", "Y", "Z"}));
  params.linkParameters(k_ChangeOrigin, k_Origin, true);
  params.linkParameters(k_ChangeSpacing, k_Spacing, true);
}

/**
 * @brief Checks an origin or spacing and identifies the input file in the error.
 * @param filePath Path to the input file.
 * @param value Origin or spacing in X, Y, Z order.
 * @param isSpacing True to check spacing, false to check origin.
 * @param overridden True if the value comes from an override. This changes the error message.
 * @return Success, -1556 for invalid spacing, or -1557 for invalid origin.
 */
[[nodiscard]] inline Result<> ValidateReaderValue(const std::filesystem::path& filePath, const FloatVec3& value, bool isSpacing, bool overridden = false)
{
  auto result = isSpacing ? ImageGeom::ValidateSpacing(value) : ImageGeom::ValidateOrigin(value);
  if(result.invalid())
  {
    auto& error = result.errors().front();
    const auto label = isSpacing ? k_SetSpacingLabel : k_SetOriginLabel;
    if(overridden)
    {
      error.message = fmt::format("'{}' has invalid values for input file '{}'. Error Message: {}", label.view(), filePath.string(), error.message);
    }
    else
    {
      error.message = FormatReaderValueError(filePath, isSpacing ? "spacing" : "origin", error.message, label);
    }
  }
  return result;
}

struct GeometryOverrides
{
  std::optional<FloatVec3> Origin;
  std::optional<FloatVec3> Spacing;

  /**
   * @brief Gets the enabled origin and spacing overrides.
   * @param args Reader filter arguments.
   * @return An optional value for each enabled override. Validation occurs when the overrides are applied.
   */
  [[nodiscard]] static GeometryOverrides FromArguments(const Arguments& args)
  {
    GeometryOverrides overrides;
    if(args.value<bool>(k_ChangeOrigin))
    {
      overrides.Origin = FloatVec3(args.value<VectorFloat32Parameter::ValueType>(k_Origin));
    }
    if(args.value<bool>(k_ChangeSpacing))
    {
      overrides.Spacing = FloatVec3(args.value<VectorFloat32Parameter::ValueType>(k_Spacing));
    }
    return overrides;
  }

  /**
   * @brief Selects the effective image metadata before any geometry or crop calculation.
   * @tparam OriginType Writable origin vector with three components.
   * @tparam SpacingType Writable spacing vector with three components.
   * @param path Path to the input file.
   * @param origin Stored origin on input and effective origin on success.
   * @param spacing Stored spacing on input and effective spacing on success.
   * @return Success, -1556 for invalid spacing, or -1557 for invalid origin. Both output vectors are unchanged on error.
   */
  template <class OriginType, class SpacingType>
  [[nodiscard]] Result<> apply(const std::filesystem::path& path, OriginType& origin, SpacingType& spacing) const
  {
    const auto effectiveOrigin = Origin.value_or(FloatVec3{origin[0], origin[1], origin[2]});
    const auto effectiveSpacing = Spacing.value_or(FloatVec3{spacing[0], spacing[1], spacing[2]});
    if(auto result = ValidateReaderValue(path, effectiveOrigin, false, Origin.has_value()); result.invalid())
    {
      return result;
    }
    if(auto result = ValidateReaderValue(path, effectiveSpacing, true, Spacing.has_value()); result.invalid())
    {
      return result;
    }
    for(usize index = 0; index < 3; ++index)
    {
      origin[index] = effectiveOrigin[index];
      spacing[index] = effectiveSpacing[index];
    }
    return {};
  }
};
/**
 * @brief Resolves source-space crop metadata shared by preflight and execution.
 * Postprocessed settings preserve valid native metadata during cropping. An enabled
 * override replaces invalid native metadata before any geometry calculation.
 * @param path Path to the input file.
 * @param overrides Enabled origin and spacing overrides.
 * @param processing When to apply the overrides.
 * @param centerOrigin Whether to center the geometry when preprocessing an origin override.
 * @param dims Source dimensions in X, Y, Z order.
 * @param origin Stored origin on input and crop origin on success.
 * @param spacing Stored spacing on input and crop spacing on success.
 * @return Success, -1556 for invalid spacing, or -1557 for invalid origin.
 * If centering fails, the output vectors contain the effective spacing and the computed origin.
 */
[[nodiscard]] inline Result<> ResolveCropGeometry(const std::filesystem::path& path, const GeometryOverrides& overrides, OriginSpacingProcessing processing, bool centerOrigin, const SizeVec3& dims,
                                                  FloatVec3& origin, FloatVec3& spacing)
{
  auto cropOverrides = overrides;
  if(processing == OriginSpacingProcessing::Postprocessed)
  {
    if(ImageGeom::ValidateOrigin(origin).valid())
    {
      cropOverrides.Origin.reset();
    }
    if(ImageGeom::ValidateSpacing(spacing).valid())
    {
      cropOverrides.Spacing.reset();
    }
  }
  if(auto result = cropOverrides.apply(path, origin, spacing); result.invalid())
  {
    return result;
  }
  if(processing == OriginSpacingProcessing::Preprocessed && centerOrigin && cropOverrides.Origin.has_value())
  {
    for(usize i = 0; i < 3; ++i)
    {
      origin[i] = -0.5F * spacing[i] * static_cast<float32>(dims[i]);
    }
    return ValidateReaderValue(path, origin, false, true);
  }
  return {};
}

/**
 * @brief Applies the final postprocessed geometry settings to the cropped image.
 * @param path Path to the input file.
 * @param overrides Enabled origin and spacing overrides.
 * @param centerOrigin Whether to center the geometry when an origin override is enabled.
 * @param dims Cropped dimensions in X, Y, Z order.
 * @param origin Crop origin on input and output origin on success.
 * @param spacing Crop spacing on input and output spacing on success.
 * @return Success, -1556 for invalid spacing, or -1557 for invalid origin.
 * If centering fails, the output vectors contain the effective spacing and the computed origin.
 */
[[nodiscard]] inline Result<> ApplyPostprocessedGeometry(const std::filesystem::path& path, const GeometryOverrides& overrides, bool centerOrigin, const SizeVec3& dims, FloatVec3& origin,
                                                         FloatVec3& spacing)
{
  if(auto result = overrides.apply(path, origin, spacing); result.invalid())
  {
    return result;
  }
  if(centerOrigin && overrides.Origin.has_value())
  {
    for(usize i = 0; i < 3; ++i)
    {
      origin[i] = -0.5F * spacing[i] * static_cast<float32>(dims[i]);
    }
    return ValidateReaderValue(path, origin, false, true);
  }
  return {};
}
} // namespace nx::core::ImageIO
