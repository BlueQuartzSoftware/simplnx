#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ThresholdMaximumConnectedComponentsImageFilter
 * @brief Searches native finite bounds with a component-count heuristic and an absolute two-intensity-unit stop.
 *
 * Finite scalar input values define the search interval. The uint8 output uses Inside Value and Outside Value.
 * The search counts nonzero output components. Component run tables grow with image topology.
 */
class NXIMAGEPROCESSING_EXPORT ThresholdMaximumConnectedComponentsImageFilter : public IFilter
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
