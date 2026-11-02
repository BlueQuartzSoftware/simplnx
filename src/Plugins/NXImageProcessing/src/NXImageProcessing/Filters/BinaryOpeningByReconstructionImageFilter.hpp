#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryOpeningByReconstructionImageFilter
 * @brief ITK-free, out-of-core-capable binary opening by reconstruction: a binary erosion followed by a binary
 *        reconstruction-by-dilation of the eroded image under the original image.
 *
 * Binary opening by reconstruction removes foreground objects smaller than the structuring element (like a binary
 * opening) but, unlike a plain binary opening, restores the exact shape of the objects that survive the erosion
 * (the reconstruction regrows the full connected component of each surviving object). Only voxels equal to the
 * Foreground Value are treated as foreground; every surviving component is painted the foreground value and
 * everything else the background value.
 *
 * Integer element types only; the input must be single-component (scalar) and strictly binary (only the foreground
 * or background value). Output type == input type. This matches the legacy ITK Binary Opening By Reconstruction
 * Image Filter exactly, and is out-of-core capable (the erosion and reconstruction stream through bounded-memory
 * scratch stores).
 */
class IMAGEPROCESSING_EXPORT BinaryOpeningByReconstructionImageFilter : public IFilter
{
public:
  BinaryOpeningByReconstructionImageFilter() = default;
  ~BinaryOpeningByReconstructionImageFilter() noexcept override = default;

  BinaryOpeningByReconstructionImageFilter(const BinaryOpeningByReconstructionImageFilter&) = delete;
  BinaryOpeningByReconstructionImageFilter(BinaryOpeningByReconstructionImageFilter&&) noexcept = delete;
  BinaryOpeningByReconstructionImageFilter& operator=(const BinaryOpeningByReconstructionImageFilter&) = delete;
  BinaryOpeningByReconstructionImageFilter& operator=(BinaryOpeningByReconstructionImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryOpeningByReconstructionImageFilter, "ebb01ba2-ec09-4eee-915f-2d51ba280a18");
