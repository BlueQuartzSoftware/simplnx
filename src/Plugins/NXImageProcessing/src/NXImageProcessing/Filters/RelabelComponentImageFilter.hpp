#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RelabelComponentImageFilter
 * @brief ITK-free, out-of-core-capable relabeling of an ALREADY-LABELED integer image.
 *
 * Computes each label's pixel count, optionally sorts the labels by size (largest first; ties keep the original
 * label order) when SortByObjectSize is on, discards components smaller than MinimumObjectSize (mapped to
 * background 0), and reassigns consecutive labels 1, 2, 3, ... in that order. This filter does NOT perform
 * connected-component labeling itself -- see ConnectedComponentImageFilter for that; the input here is assumed to
 * already be labeled. The input may be ANY integer scalar type; the output has the SAME type as the input. Matches
 * the legacy ITK Relabel Component Image Filter exactly (byte-identical label values).
 */
class IMAGEPROCESSING_EXPORT RelabelComponentImageFilter : public IFilter
{
public:
  RelabelComponentImageFilter() = default;
  ~RelabelComponentImageFilter() noexcept override = default;

  RelabelComponentImageFilter(const RelabelComponentImageFilter&) = delete;
  RelabelComponentImageFilter(RelabelComponentImageFilter&&) noexcept = delete;
  RelabelComponentImageFilter& operator=(const RelabelComponentImageFilter&) = delete;
  RelabelComponentImageFilter& operator=(RelabelComponentImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_MinimumObjectSize_Key = "minimum_object_size";
  static constexpr StringLiteral k_SortByObjectSize_Key = "sort_by_object_size";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RelabelComponentImageFilter, "de830a9e-1fc0-47fa-9c2d-895366af832a");
