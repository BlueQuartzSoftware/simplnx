#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryDilateImageFilter
 * @brief ITK-free, out-of-core-capable binary dilation of an image.
 *
 * Only voxels equal to the Foreground Value are treated as foreground; every other value is background.
 * A voxel becomes foreground when ANY neighbor covered by the flat structuring element (kernel) centered on
 * it is foreground, and background otherwise, so the output is strictly {Foreground, Background}. The kernel
 * type (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Binary Dilate Image Filter; unlike
 * the legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT BinaryDilateImageFilter : public IFilter
{
public:
  BinaryDilateImageFilter() = default;
  ~BinaryDilateImageFilter() noexcept override = default;

  BinaryDilateImageFilter(const BinaryDilateImageFilter&) = delete;
  BinaryDilateImageFilter(BinaryDilateImageFilter&&) noexcept = delete;
  BinaryDilateImageFilter& operator=(const BinaryDilateImageFilter&) = delete;
  BinaryDilateImageFilter& operator=(BinaryDilateImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryDilateImageFilter, "d9e33b21-670e-4bab-bc42-f5bbf38891ab");
