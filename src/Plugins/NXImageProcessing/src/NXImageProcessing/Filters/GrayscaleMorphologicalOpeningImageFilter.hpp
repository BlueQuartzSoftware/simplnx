#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GrayscaleMorphologicalOpeningImageFilter
 * @brief ITK-free, out-of-core-capable grayscale morphological opening of an image.
 *
 * Opening is a grayscale erosion followed by a grayscale dilation through the same flat structuring element;
 * it removes bright features smaller than the structuring element while preserving overall shape. The kernel
 * type (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Grayscale Morphological Opening
 * Image Filter; unlike the legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty)
 * thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT GrayscaleMorphologicalOpeningImageFilter : public IFilter
{
public:
  GrayscaleMorphologicalOpeningImageFilter() = default;
  ~GrayscaleMorphologicalOpeningImageFilter() noexcept override = default;

  GrayscaleMorphologicalOpeningImageFilter(const GrayscaleMorphologicalOpeningImageFilter&) = delete;
  GrayscaleMorphologicalOpeningImageFilter(GrayscaleMorphologicalOpeningImageFilter&&) noexcept = delete;
  GrayscaleMorphologicalOpeningImageFilter& operator=(const GrayscaleMorphologicalOpeningImageFilter&) = delete;
  GrayscaleMorphologicalOpeningImageFilter& operator=(GrayscaleMorphologicalOpeningImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_SafeBorder_Key = "safe_border";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GrayscaleMorphologicalOpeningImageFilter, "74ad4b11-a144-4986-9259-2daf6da22fc0");
