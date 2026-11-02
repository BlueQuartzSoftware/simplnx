#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class MorphologicalGradientImageFilter
 * @brief ITK-free, out-of-core-capable grayscale morphological gradient of an image.
 *
 * The morphological gradient is the grayscale dilation minus the grayscale erosion through the same flat
 * structuring element, giving the local intensity range (edge strength) at each voxel. The kernel type
 * (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Morphological Gradient Image Filter;
 * unlike the legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT MorphologicalGradientImageFilter : public IFilter
{
public:
  MorphologicalGradientImageFilter() = default;
  ~MorphologicalGradientImageFilter() noexcept override = default;

  MorphologicalGradientImageFilter(const MorphologicalGradientImageFilter&) = delete;
  MorphologicalGradientImageFilter(MorphologicalGradientImageFilter&&) noexcept = delete;
  MorphologicalGradientImageFilter& operator=(const MorphologicalGradientImageFilter&) = delete;
  MorphologicalGradientImageFilter& operator=(MorphologicalGradientImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MorphologicalGradientImageFilter, "c199ce3f-501e-453d-92c9-145d7958a843");
