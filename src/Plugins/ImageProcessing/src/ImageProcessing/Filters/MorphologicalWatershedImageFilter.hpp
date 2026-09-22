#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class MorphologicalWatershedImageFilter
 * @brief ITK-free, out-of-core-capable morphological watershed segmentation.
 *
 * A composite that reproduces ITK's MorphologicalWatershedImageFilter: (1) optionally suppress shallow minima with
 * an HMinima transform of depth @c Level; (2) find the regional minima of that image; (3) label the minima with
 * connected components to form the flood markers; (4) run a marker-controlled hierarchical-queue (FAH) watershed
 * flood, optionally marking the watershed lines. The input may be ANY scalar type; the output is a FIXED uint32 label
 * image regardless of the input element type. Matches the legacy ITK Morphological Watershed Image Filter exactly
 * (byte-identical label values). Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT MorphologicalWatershedImageFilter : public IFilter
{
public:
  MorphologicalWatershedImageFilter() = default;
  ~MorphologicalWatershedImageFilter() noexcept override = default;

  MorphologicalWatershedImageFilter(const MorphologicalWatershedImageFilter&) = delete;
  MorphologicalWatershedImageFilter(MorphologicalWatershedImageFilter&&) noexcept = delete;
  MorphologicalWatershedImageFilter& operator=(const MorphologicalWatershedImageFilter&) = delete;
  MorphologicalWatershedImageFilter& operator=(MorphologicalWatershedImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Level_Key = "level";
  static constexpr StringLiteral k_MarkWatershedLine_Key = "mark_watershed_line";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MorphologicalWatershedImageFilter, "d7526386-6ab5-4429-b51d-5e6540af5241");
