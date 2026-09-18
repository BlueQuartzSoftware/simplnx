#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryProjectionImageFilter
 * @brief ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Projection Image
 *        Filter. Collapses one axis of a scalar Image Geometry to size 1: each output voxel is the
 *        foreground value if ANY voxel along the projected axis equals the foreground value, otherwise
 *        the background value.
 */
class IMAGEPROCESSING_EXPORT BinaryProjectionImageFilter : public IFilter
{
public:
  BinaryProjectionImageFilter() = default;
  ~BinaryProjectionImageFilter() noexcept override = default;

  BinaryProjectionImageFilter(const BinaryProjectionImageFilter&) = delete;
  BinaryProjectionImageFilter(BinaryProjectionImageFilter&&) noexcept = delete;
  BinaryProjectionImageFilter& operator=(const BinaryProjectionImageFilter&) = delete;
  BinaryProjectionImageFilter& operator=(BinaryProjectionImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ProjectionDimension_Key = "projection_dimension";
  static constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageGeomName_Key = "output_image_geometry_name";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryProjectionImageFilter, "24044b7e-d9e3-4908-8f92-e7318d51c9c8");
