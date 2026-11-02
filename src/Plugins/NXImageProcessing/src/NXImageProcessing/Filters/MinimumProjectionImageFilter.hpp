#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class MinimumProjectionImageFilter
 * @brief ITK-free, out-of-core-capable reimplementation of the legacy ITK Minimum Projection Image
 *        Filter. Collapses one axis of a scalar Image Geometry by taking, for each column along the
 *        projected axis, the minimum value; the projected axis becomes size 1.
 */
class IMAGEPROCESSING_EXPORT MinimumProjectionImageFilter : public IFilter
{
public:
  MinimumProjectionImageFilter() = default;
  ~MinimumProjectionImageFilter() noexcept override = default;

  MinimumProjectionImageFilter(const MinimumProjectionImageFilter&) = delete;
  MinimumProjectionImageFilter(MinimumProjectionImageFilter&&) noexcept = delete;
  MinimumProjectionImageFilter& operator=(const MinimumProjectionImageFilter&) = delete;
  MinimumProjectionImageFilter& operator=(MinimumProjectionImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ProjectionDimension_Key = "projection_dimension";
  static constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageGeomName_Key = "output_image_geometry_name";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MinimumProjectionImageFilter, "4c7cdf9b-0cd6-40b0-b0f5-e30aa9983513");
