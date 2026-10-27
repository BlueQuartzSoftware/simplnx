#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ThresholdMaximumConnectedComponentsImageFilter
 * @brief Searches native finite bounds with a component-count heuristic and an absolute two-intensity-unit stop.
 *
 * The input may be ANY scalar type; the output is a FIXED uint8 binary image (Inside Value / Outside Value)
 * regardless of the input element type. Safe midpoint arithmetic can change legacy segmentations. Image NaN and
 * infinities support Error or warned Outside participation. The shared traversal supports both storage types;
 * component run tables still grow with image topology. The search does not guarantee a global maximum.
 */
class IMAGEPROCESSING_EXPORT ThresholdMaximumConnectedComponentsImageFilter : public IFilter
{
public:
  ThresholdMaximumConnectedComponentsImageFilter() = default;
  ~ThresholdMaximumConnectedComponentsImageFilter() noexcept override = default;

  ThresholdMaximumConnectedComponentsImageFilter(const ThresholdMaximumConnectedComponentsImageFilter&) = delete;
  ThresholdMaximumConnectedComponentsImageFilter(ThresholdMaximumConnectedComponentsImageFilter&&) noexcept = delete;
  ThresholdMaximumConnectedComponentsImageFilter& operator=(const ThresholdMaximumConnectedComponentsImageFilter&) = delete;
  ThresholdMaximumConnectedComponentsImageFilter& operator=(ThresholdMaximumConnectedComponentsImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_MinimumObjectSizeInPixels_Key = "minimum_object_size_in_pixels";
  static constexpr StringLiteral k_UpperBoundary_Key = "upper_boundary";
  static constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static constexpr StringLiteral k_OutsideValue_Key = "outside_value";
  static constexpr StringLiteral k_NonfiniteInputHandling_Key = "nonfinite_input_handling_index";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ThresholdMaximumConnectedComponentsImageFilter, "1d103fda-8d59-49a6-b586-316ec06bf581");
