#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SumProjectionImageFilter
 * @brief ITK-free, out-of-core-capable reimplementation of the legacy ITK Sum Projection Image Filter.
 *        Collapses one axis of a scalar Image Geometry to size 1: each output voxel is the sum of the
 *        voxels along the projected axis. The input may be any scalar numeric type; the output is always
 *        Float64 (double), matching the legacy filter.
 */
class IMAGEPROCESSING_EXPORT SumProjectionImageFilter : public IFilter
{
public:
  SumProjectionImageFilter() = default;
  ~SumProjectionImageFilter() noexcept override = default;

  SumProjectionImageFilter(const SumProjectionImageFilter&) = delete;
  SumProjectionImageFilter(SumProjectionImageFilter&&) noexcept = delete;
  SumProjectionImageFilter& operator=(const SumProjectionImageFilter&) = delete;
  SumProjectionImageFilter& operator=(SumProjectionImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SumProjectionImageFilter, "dfdd5b68-7623-445d-8cb2-1775354df7ea");
