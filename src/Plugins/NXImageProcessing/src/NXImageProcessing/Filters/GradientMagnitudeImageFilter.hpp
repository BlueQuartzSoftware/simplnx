#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GradientMagnitudeImageFilter
 * @brief ITK-free, out-of-core-capable gradient magnitude via a single-pass central-difference derivative.
 *
 * Computes sqrt(sum over axes of (0.5*(src[axis-1]-src[axis+1]))^2), i.e. the magnitude of the order-1 central-
 * difference gradient at every voxel. When Use Image Spacing is enabled, each axis's derivative is scaled by
 * 1/spacing[axis] so the result is in physical space; otherwise it is computed in isotropic voxel space. The input
 * may be ANY scalar numeric type (integer OR floating point); the output is a FIXED float32 image regardless of the
 * input element type. Matches the legacy ITK Gradient Magnitude Image Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT GradientMagnitudeImageFilter : public IFilter
{
public:
  GradientMagnitudeImageFilter() = default;
  ~GradientMagnitudeImageFilter() noexcept override = default;

  GradientMagnitudeImageFilter(const GradientMagnitudeImageFilter&) = delete;
  GradientMagnitudeImageFilter(GradientMagnitudeImageFilter&&) noexcept = delete;
  GradientMagnitudeImageFilter& operator=(const GradientMagnitudeImageFilter&) = delete;
  GradientMagnitudeImageFilter& operator=(GradientMagnitudeImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_UseImageSpacing_Key = "use_image_spacing";

  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GradientMagnitudeImageFilter, "e9bc5ba0-a6db-4537-aff3-86d8b535327e");
