#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT OtsuMultipleThresholdsImageFilter : public IFilter
{
public:
  OtsuMultipleThresholdsImageFilter() = default;
  ~OtsuMultipleThresholdsImageFilter() noexcept override = default;

  OtsuMultipleThresholdsImageFilter(const OtsuMultipleThresholdsImageFilter&) = delete;
  OtsuMultipleThresholdsImageFilter(OtsuMultipleThresholdsImageFilter&&) noexcept = delete;
  OtsuMultipleThresholdsImageFilter& operator=(const OtsuMultipleThresholdsImageFilter&) = delete;
  OtsuMultipleThresholdsImageFilter& operator=(OtsuMultipleThresholdsImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_NumberOfThresholds_Key = "number_of_thresholds";
  static constexpr StringLiteral k_LabelOffset_Key = "label_offset";
  static constexpr StringLiteral k_NumberOfHistogramBins_Key = "number_of_histogram_bins";
  static constexpr StringLiteral k_ValleyEmphasis_Key = "valley_emphasis";
  static constexpr StringLiteral k_ReturnBinMidpoint_Key = "return_bin_midpoint";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, OtsuMultipleThresholdsImageFilter, "1aaedd47-62aa-4310-a353-af1a40f6ee82");
