#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class OpeningByReconstructionImageFilter
 * @brief ITK-free, out-of-core-capable grayscale opening by reconstruction: a morphological erosion followed by a
 *        geodesic reconstruction-by-dilation under the original image.
 *
 * Opening by reconstruction removes bright structures smaller than the structuring element (like a plain grayscale
 * opening) but, unlike a plain opening, restores the exact contours of the structures that survive the erosion
 * (the reconstruction regrows them under the original image as a mask). When Preserve Intensities is on, the
 * surviving structures are additionally re-valued to their original input intensities rather than the eroded ones.
 *
 * Output type == input type; the input must be single-component (scalar). This matches the legacy ITK Opening By
 * Reconstruction Image Filter exactly for integer and float types, and is out-of-core capable: the erosion and
 * both reconstruction passes stream through bounded-memory scratch stores.
 */
class IMAGEPROCESSING_EXPORT OpeningByReconstructionImageFilter : public IFilter
{
public:
  OpeningByReconstructionImageFilter() = default;
  ~OpeningByReconstructionImageFilter() noexcept override = default;

  OpeningByReconstructionImageFilter(const OpeningByReconstructionImageFilter&) = delete;
  OpeningByReconstructionImageFilter(OpeningByReconstructionImageFilter&&) noexcept = delete;
  OpeningByReconstructionImageFilter& operator=(const OpeningByReconstructionImageFilter&) = delete;
  OpeningByReconstructionImageFilter& operator=(OpeningByReconstructionImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_PreserveIntensities_Key = "preserve_intensities";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, OpeningByReconstructionImageFilter, "652094ae-9781-4164-a4eb-afceb343418f");
