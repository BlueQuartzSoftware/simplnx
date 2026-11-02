#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BlackTopHatImageFilter
 * @brief ITK-free, out-of-core-capable grayscale black top-hat transform of an image.
 *
 * The black top-hat is the grayscale morphological closing minus the input, isolating the dark features
 * smaller than the flat structuring element (those the closing filled). The kernel type
 * (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Black Top Hat Image Filter; unlike the
 * legacy SimpleITK wrapper, the Annulus kernel produces a proper (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT BlackTopHatImageFilter : public IFilter
{
public:
  BlackTopHatImageFilter() = default;
  ~BlackTopHatImageFilter() noexcept override = default;

  BlackTopHatImageFilter(const BlackTopHatImageFilter&) = delete;
  BlackTopHatImageFilter(BlackTopHatImageFilter&&) noexcept = delete;
  BlackTopHatImageFilter& operator=(const BlackTopHatImageFilter&) = delete;
  BlackTopHatImageFilter& operator=(BlackTopHatImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BlackTopHatImageFilter, "a2e561ae-4728-495a-84a3-84169553d86c");
