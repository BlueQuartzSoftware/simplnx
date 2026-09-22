#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SmoothingRecursiveGaussianImageFilter
 * @brief ITK-free, out-of-core-capable separable recursive-Gaussian (Deriche) smoothing.
 *
 * Convolves the input with a Gaussian (per-axis sigma) using the Deriche 4th-order recursive IIR approximation.
 * The input may be any SIGNED scalar type (int8/16/32/64 or float32/64); the output has the SAME type as the input.
 * Matches the legacy ITK Smoothing Recursive Gaussian Image Filter exactly for integer/float64 inputs.
 */
class IMAGEPROCESSING_EXPORT SmoothingRecursiveGaussianImageFilter : public IFilter
{
public:
  SmoothingRecursiveGaussianImageFilter() = default;
  ~SmoothingRecursiveGaussianImageFilter() noexcept override = default;

  SmoothingRecursiveGaussianImageFilter(const SmoothingRecursiveGaussianImageFilter&) = delete;
  SmoothingRecursiveGaussianImageFilter(SmoothingRecursiveGaussianImageFilter&&) noexcept = delete;
  SmoothingRecursiveGaussianImageFilter& operator=(const SmoothingRecursiveGaussianImageFilter&) = delete;
  SmoothingRecursiveGaussianImageFilter& operator=(SmoothingRecursiveGaussianImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SmoothingRecursiveGaussianImageFilter, "435d2425-066a-4a0d-a069-2d40d6a6349e");
