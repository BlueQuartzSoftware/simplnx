#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class DoubleThresholdImageFilter
 * @brief ITK-free, out-of-core-capable double-threshold (hysteresis-style) binary segmentation for scalar images.
 *
 * Segments a scalar image with FOUR thresholds T1 <= T2 <= T3 <= T4: a narrow marker band [T2,T3] seeds the
 * foreground, and a wide mask band [T1,T4] defines where that foreground may grow. The marker is reconstructed
 * (by geodesic dilation) under the mask to convergence, so any connected component of the wide band that contains
 * at least one narrow-band voxel becomes InsideValue; everything else becomes OutsideValue. The input may be ANY
 * scalar type; the output is a FIXED uint8 label image regardless of the input element type. Matches the legacy
 * ITK Double Threshold Image Filter exactly (byte-identical output values). Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT DoubleThresholdImageFilter : public IFilter
{
public:
  DoubleThresholdImageFilter() = default;
  ~DoubleThresholdImageFilter() noexcept override = default;

  DoubleThresholdImageFilter(const DoubleThresholdImageFilter&) = delete;
  DoubleThresholdImageFilter(DoubleThresholdImageFilter&&) noexcept = delete;
  DoubleThresholdImageFilter& operator=(const DoubleThresholdImageFilter&) = delete;
  DoubleThresholdImageFilter& operator=(DoubleThresholdImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Threshold1_Key = "threshold1";
  static constexpr StringLiteral k_Threshold2_Key = "threshold2";
  static constexpr StringLiteral k_Threshold3_Key = "threshold3";
  static constexpr StringLiteral k_Threshold4_Key = "threshold4";
  static constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static constexpr StringLiteral k_OutsideValue_Key = "outside_value";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, DoubleThresholdImageFilter, "8350bea5-7958-4a45-9cd6-b996e56b3749");
