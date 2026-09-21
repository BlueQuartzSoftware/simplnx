#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ZeroCrossingImageFilter
 * @brief ITK-free, out-of-core-capable zero-crossing (sign-change) edge marker for signed scalar images.
 *
 * Finds the pixels closest to the zero crossings (sign changes) in a SIGNED scalar image: each voxel is compared
 * against its axial face neighbors (x±1, y±1, z±1) and, when it lies on the closer-to-zero side of a sign change,
 * is labeled with the ForegroundValue; all other voxels get the BackgroundValue. The input may be ANY signed
 * scalar type (int8/int16/int32/int64/float32/float64); the output is a FIXED uint8 label image regardless of the
 * input element type. Matches the legacy ITK Zero Crossing Image Filter exactly (byte-identical output values).
 * Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT ZeroCrossingImageFilter : public IFilter
{
public:
  ZeroCrossingImageFilter() = default;
  ~ZeroCrossingImageFilter() noexcept override = default;

  ZeroCrossingImageFilter(const ZeroCrossingImageFilter&) = delete;
  ZeroCrossingImageFilter(ZeroCrossingImageFilter&&) noexcept = delete;
  ZeroCrossingImageFilter& operator=(const ZeroCrossingImageFilter&) = delete;
  ZeroCrossingImageFilter& operator=(ZeroCrossingImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ZeroCrossingImageFilter, "8a22a748-efcc-4982-82ef-0e8e89c92f5b");
