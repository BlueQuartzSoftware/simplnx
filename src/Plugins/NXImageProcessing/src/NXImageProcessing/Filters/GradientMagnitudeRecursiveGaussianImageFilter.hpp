#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GradientMagnitudeRecursiveGaussianImageFilter
 * @brief ITK-free, out-of-core-capable gradient magnitude via separable recursive-Gaussian derivatives.
 *
 * Computes sqrt(sum over axes of (d/dx_i of the Gaussian-smoothed image)^2) using the Deriche 4th-order recursive IIR
 * approximation. Sigma is a single scalar shared by every axis, in the units of image spacing. The input may be ANY
 * scalar numeric type (integer OR floating point); the output is a FIXED float32 image regardless of the input element
 * type. Matches the legacy ITK Gradient Magnitude Recursive Gaussian Image Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT GradientMagnitudeRecursiveGaussianImageFilter : public IFilter
{
public:
  GradientMagnitudeRecursiveGaussianImageFilter() = default;
  ~GradientMagnitudeRecursiveGaussianImageFilter() noexcept override = default;

  GradientMagnitudeRecursiveGaussianImageFilter(const GradientMagnitudeRecursiveGaussianImageFilter&) = delete;
  GradientMagnitudeRecursiveGaussianImageFilter(GradientMagnitudeRecursiveGaussianImageFilter&&) noexcept = delete;
  GradientMagnitudeRecursiveGaussianImageFilter& operator=(const GradientMagnitudeRecursiveGaussianImageFilter&) = delete;
  GradientMagnitudeRecursiveGaussianImageFilter& operator=(GradientMagnitudeRecursiveGaussianImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Sigma_Key = "sigma";
  static constexpr StringLiteral k_NormalizeAcrossScale_Key = "normalize_across_scale";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GradientMagnitudeRecursiveGaussianImageFilter, "d9b71e00-6c15-4cb5-92bd-a8c2d9be57f2");
