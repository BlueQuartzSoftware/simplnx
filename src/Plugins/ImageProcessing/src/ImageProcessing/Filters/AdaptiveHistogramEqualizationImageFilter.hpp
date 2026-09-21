#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class AdaptiveHistogramEqualizationImageFilter
 * @brief ITK-free, out-of-core-capable adaptive histogram equalization (Stark 2000 contrast enhancement).
 *
 * For each voxel the filter normalizes the whole-image gray range to [-0.5, 0.5], then over its box window
 * accumulates a cumulative function parameterized by Alpha (0 -> classic adaptive histogram equalization,
 * 1 -> unsharp mask) and Beta (balance toward the local mean vs. pass-through; alpha=beta=1 reproduces the
 * input). Out-of-bounds neighbors are ignored and the valid part over-weighted, matching ITK exactly (so a
 * 2D image needs no special handling). Output type == input type; all 10 scalar types are supported.
 *
 * This matches the legacy ITK Adaptive Histogram Equalization Image Filter within a small tolerance: bit-for-
 * bit parity is impossible because ITK accumulates over an unordered_map in hash order. A constant image is
 * passed through unchanged (ITK divides 0/0 -> NaN there). The filter is now out-of-core capable (the legacy
 * filter rejected chunked arrays).
 */
class IMAGEPROCESSING_EXPORT AdaptiveHistogramEqualizationImageFilter : public IFilter
{
public:
  AdaptiveHistogramEqualizationImageFilter() = default;
  ~AdaptiveHistogramEqualizationImageFilter() noexcept override = default;

  AdaptiveHistogramEqualizationImageFilter(const AdaptiveHistogramEqualizationImageFilter&) = delete;
  AdaptiveHistogramEqualizationImageFilter(AdaptiveHistogramEqualizationImageFilter&&) noexcept = delete;
  AdaptiveHistogramEqualizationImageFilter& operator=(const AdaptiveHistogramEqualizationImageFilter&) = delete;
  AdaptiveHistogramEqualizationImageFilter& operator=(AdaptiveHistogramEqualizationImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Radius_Key = "radius";
  static constexpr StringLiteral k_Alpha_Key = "alpha";
  static constexpr StringLiteral k_Beta_Key = "beta";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, AdaptiveHistogramEqualizationImageFilter, "303249e9-4623-427a-af90-c837d38133d7");
