#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT BinaryThresholdImageFilter : public IFilter
{
public:
  BinaryThresholdImageFilter() = default;
  ~BinaryThresholdImageFilter() noexcept override = default;

  BinaryThresholdImageFilter(const BinaryThresholdImageFilter&) = delete;
  BinaryThresholdImageFilter(BinaryThresholdImageFilter&&) noexcept = delete;
  BinaryThresholdImageFilter& operator=(const BinaryThresholdImageFilter&) = delete;
  BinaryThresholdImageFilter& operator=(BinaryThresholdImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_LowerThreshold_Key = "lower_threshold";
  static constexpr StringLiteral k_UpperThreshold_Key = "upper_threshold";
  static constexpr StringLiteral k_InsideValue_Key = "inside_value";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryThresholdImageFilter, "4bf09378-e159-4437-9ce2-3f4c3b5ad7ae");
