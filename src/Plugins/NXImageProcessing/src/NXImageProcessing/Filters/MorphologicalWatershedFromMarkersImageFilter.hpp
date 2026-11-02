#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class IMAGEPROCESSING_EXPORT MorphologicalWatershedFromMarkersImageFilter : public IFilter
{
public:
  MorphologicalWatershedFromMarkersImageFilter() = default;
  ~MorphologicalWatershedFromMarkersImageFilter() noexcept override = default;

  MorphologicalWatershedFromMarkersImageFilter(const MorphologicalWatershedFromMarkersImageFilter&) = delete;
  MorphologicalWatershedFromMarkersImageFilter(MorphologicalWatershedFromMarkersImageFilter&&) noexcept = delete;
  MorphologicalWatershedFromMarkersImageFilter& operator=(const MorphologicalWatershedFromMarkersImageFilter&) = delete;
  MorphologicalWatershedFromMarkersImageFilter& operator=(MorphologicalWatershedFromMarkersImageFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_MarkerImageDataPath_Key = "marker_image_data_path";
  static constexpr StringLiteral k_MarkWatershedLine_Key = "mark_watershed_line";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MorphologicalWatershedFromMarkersImageFilter, "5a489229-9421-4191-a059-04943147f7ed");
