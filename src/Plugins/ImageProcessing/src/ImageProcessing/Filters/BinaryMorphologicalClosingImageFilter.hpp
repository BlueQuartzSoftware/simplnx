#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryMorphologicalClosingImageFilter
 * @brief ITK-free, out-of-core-capable binary morphological closing of an image.
 *
 * Closing is a binary dilation followed by a binary erosion through the same flat structuring element (kernel);
 * it fills background holes/gaps smaller than the structuring element while preserving the overall shape of
 * larger structures. Only voxels equal to the Foreground Value are treated as foreground; every other value is
 * background, so the output is strictly {Foreground, Background}. Because closing is extensive it never adds new
 * background, so (like the legacy ITK filter) it exposes no Background Value: it uses an internal background
 * derived from the Foreground Value (0, or the type maximum when the Foreground Value is 0, so the two stay
 * distinct). NOTE: a binary-input safeguard requires every input voxel to already equal the Foreground Value or
 * that derived background and rejects anything else (-8002); this is intentionally STRICTER than ITK, which
 * would process a multi-label input and preserve its non-foreground values. Boundary handling near the image
 * edge is controlled by the Safe Border
 * parameter. The kernel type (Annulus/Ball/Box/Cross) and per-axis radius match the legacy ITK Binary
 * Morphological Closing Image Filter; unlike the legacy SimpleITK wrapper, the Annulus kernel produces a proper
 * (non-empty) thickness-1 shell.
 */
class IMAGEPROCESSING_EXPORT BinaryMorphologicalClosingImageFilter : public IFilter
{
public:
  BinaryMorphologicalClosingImageFilter() = default;
  ~BinaryMorphologicalClosingImageFilter() noexcept override = default;

  BinaryMorphologicalClosingImageFilter(const BinaryMorphologicalClosingImageFilter&) = delete;
  BinaryMorphologicalClosingImageFilter(BinaryMorphologicalClosingImageFilter&&) noexcept = delete;
  BinaryMorphologicalClosingImageFilter& operator=(const BinaryMorphologicalClosingImageFilter&) = delete;
  BinaryMorphologicalClosingImageFilter& operator=(BinaryMorphologicalClosingImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryMorphologicalClosingImageFilter, "73a22075-6760-44e8-b385-68aed4dd56c7");
