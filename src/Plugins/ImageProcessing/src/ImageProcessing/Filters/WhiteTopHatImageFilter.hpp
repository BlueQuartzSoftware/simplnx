#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WhiteTopHatImageFilter
 * @brief ITK-free, out-of-core-capable grayscale white top-hat transform of an image.
 *
 * The white top-hat is the input minus its grayscale morphological opening, isolating the bright features
 * smaller than the flat structuring element (those the opening removed). The kernel type
 * (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK White Top Hat Image Filter; unlike the
 * legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT WhiteTopHatImageFilter : public IFilter
{
public:
  WhiteTopHatImageFilter() = default;
  ~WhiteTopHatImageFilter() noexcept override = default;

  WhiteTopHatImageFilter(const WhiteTopHatImageFilter&) = delete;
  WhiteTopHatImageFilter(WhiteTopHatImageFilter&&) noexcept = delete;
  WhiteTopHatImageFilter& operator=(const WhiteTopHatImageFilter&) = delete;
  WhiteTopHatImageFilter& operator=(WhiteTopHatImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WhiteTopHatImageFilter, "c33fdeac-8313-4dcf-a3d2-e97fff859c54");
