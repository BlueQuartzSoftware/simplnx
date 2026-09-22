#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT IntensityWindowingImageFilter : public IFilter
{
public:
  IntensityWindowingImageFilter() = default;
  ~IntensityWindowingImageFilter() noexcept override = default;

  IntensityWindowingImageFilter(const IntensityWindowingImageFilter&) = delete;
  IntensityWindowingImageFilter(IntensityWindowingImageFilter&&) noexcept = delete;
  IntensityWindowingImageFilter& operator=(const IntensityWindowingImageFilter&) = delete;
  IntensityWindowingImageFilter& operator=(IntensityWindowingImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_WindowMinimum_Key = "window_minimum";
  static constexpr StringLiteral k_WindowMaximum_Key = "window_maximum";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, IntensityWindowingImageFilter, "d053ffc3-007d-42ff-9ad5-d905ebe60c74");
