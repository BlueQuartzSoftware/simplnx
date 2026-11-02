#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GrayscaleDilateImageFilter
 * @brief ITK-free, out-of-core-capable grayscale dilation of an image.
 *
 * Dilation takes the maximum of the input values covered by a flat structuring element centered on each
 * voxel. The kernel type (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Grayscale
 * Dilate Image Filter; unlike the legacy SimpleITK wrapper, the Annulus kernel produces a proper
 * (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT GrayscaleDilateImageFilter : public IFilter
{
public:
  GrayscaleDilateImageFilter() = default;
  ~GrayscaleDilateImageFilter() noexcept override = default;

  GrayscaleDilateImageFilter(const GrayscaleDilateImageFilter&) = delete;
  GrayscaleDilateImageFilter(GrayscaleDilateImageFilter&&) noexcept = delete;
  GrayscaleDilateImageFilter& operator=(const GrayscaleDilateImageFilter&) = delete;
  GrayscaleDilateImageFilter& operator=(GrayscaleDilateImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GrayscaleDilateImageFilter, "6c5fb0d3-3db5-49fe-b5d6-eddef082cd68");
