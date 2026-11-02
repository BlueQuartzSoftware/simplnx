#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT RescaleIntensityImageFilter : public IFilter
{
public:
  RescaleIntensityImageFilter() = default;
  ~RescaleIntensityImageFilter() noexcept override = default;

  RescaleIntensityImageFilter(const RescaleIntensityImageFilter&) = delete;
  RescaleIntensityImageFilter(RescaleIntensityImageFilter&&) noexcept = delete;
  RescaleIntensityImageFilter& operator=(const RescaleIntensityImageFilter&) = delete;
  RescaleIntensityImageFilter& operator=(RescaleIntensityImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_OutputMinimum_Key = "output_minimum";
  static constexpr StringLiteral k_OutputMaximum_Key = "output_maximum";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RescaleIntensityImageFilter, "30b7dbef-0d41-4feb-9e45-ed4def61eae2");
