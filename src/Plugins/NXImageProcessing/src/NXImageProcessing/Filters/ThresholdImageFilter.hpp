#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT ThresholdImageFilter : public IFilter
{
public:
  ThresholdImageFilter() = default;
  ~ThresholdImageFilter() noexcept override = default;

  ThresholdImageFilter(const ThresholdImageFilter&) = delete;
  ThresholdImageFilter(ThresholdImageFilter&&) noexcept = delete;

  ThresholdImageFilter& operator=(const ThresholdImageFilter&) = delete;
  ThresholdImageFilter& operator=(ThresholdImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Lower_Key = "lower";
  static constexpr StringLiteral k_Upper_Key = "upper";
  static constexpr StringLiteral k_OutsideValue_Key = "outside_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ThresholdImageFilter, "9c8f6b2e-3a41-4e7d-b5c9-0f21a7d84e63");
