#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryContourImageFilter
 * @brief ITK-free, out-of-core-capable extraction of the contours (borders) of the objects in a binary image.
 *
 * Only voxels equal to the Foreground Value are objects; a foreground voxel is kept (emitted as the Foreground
 * Value) only when at least one of its connectivity neighbors is not foreground, so interior foreground voxels
 * become the Background Value and only the object borders remain. Any non-foreground voxel passes its own value
 * through unchanged, matching the legacy ITK Binary Contour Image Filter exactly (which is why, unlike binary
 * morphology, this filter needs no binary-input safeguard and is faithful on labeled input). The connectivity
 * (Fully Connected off = face; on = face+edge+vertex) matches the legacy filter.
 */
class IMAGEPROCESSING_EXPORT BinaryContourImageFilter : public IFilter
{
public:
  BinaryContourImageFilter() = default;
  ~BinaryContourImageFilter() noexcept override = default;

  BinaryContourImageFilter(const BinaryContourImageFilter&) = delete;
  BinaryContourImageFilter(BinaryContourImageFilter&&) noexcept = delete;
  BinaryContourImageFilter& operator=(const BinaryContourImageFilter&) = delete;
  BinaryContourImageFilter& operator=(BinaryContourImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryContourImageFilter, "da623f5e-1c20-4c7b-a750-2bf72a421a51");
