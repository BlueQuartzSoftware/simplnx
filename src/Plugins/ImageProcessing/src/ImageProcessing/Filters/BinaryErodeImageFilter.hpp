#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryErodeImageFilter
 * @brief ITK-free, out-of-core-capable binary erosion of an image.
 *
 * Only voxels equal to the Foreground Value are treated as foreground; every other value is background.
 * A voxel remains foreground only when ALL neighbors covered by the flat structuring element (kernel)
 * centered on it are foreground, and becomes background otherwise, so the output is strictly {Foreground,
 * Background}. The kernel type (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Binary Erode
 * Image Filter; unlike the legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty)
 * thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT BinaryErodeImageFilter : public IFilter
{
public:
  BinaryErodeImageFilter() = default;
  ~BinaryErodeImageFilter() noexcept override = default;

  BinaryErodeImageFilter(const BinaryErodeImageFilter&) = delete;
  BinaryErodeImageFilter(BinaryErodeImageFilter&&) noexcept = delete;
  BinaryErodeImageFilter& operator=(const BinaryErodeImageFilter&) = delete;
  BinaryErodeImageFilter& operator=(BinaryErodeImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static constexpr StringLiteral k_BoundaryToForeground_Key = "boundary_to_foreground";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryErodeImageFilter, "cad4a094-2f2a-4ccf-8575-50f865251192");
