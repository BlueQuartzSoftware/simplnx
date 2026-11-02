#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT NormalizeToConstantImageFilter : public IFilter
{
public:
  NormalizeToConstantImageFilter() = default;
  ~NormalizeToConstantImageFilter() noexcept override = default;

  NormalizeToConstantImageFilter(const NormalizeToConstantImageFilter&) = delete;
  NormalizeToConstantImageFilter(NormalizeToConstantImageFilter&&) noexcept = delete;
  NormalizeToConstantImageFilter& operator=(const NormalizeToConstantImageFilter&) = delete;
  NormalizeToConstantImageFilter& operator=(NormalizeToConstantImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Constant_Key = "constant";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, NormalizeToConstantImageFilter, "800a65f2-8c88-41bc-8281-49b0ba07b185");
