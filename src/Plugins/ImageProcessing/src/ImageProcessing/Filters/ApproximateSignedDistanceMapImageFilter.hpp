#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ApproximateSignedDistanceMapImageFilter
 * @brief ITK-free, out-of-core-capable approximate SIGNED distance map of a mask.
 *
 * Produces a float32 image whose value at each voxel approximates the signed Euclidean distance to the boundary
 * between the Inside Value and Outside Value regions of the input mask (negative inside, positive outside). It is an
 * ITK composite: an Iso Contour Distance pass lays down an accurate narrow signed band around the boundary (iso-level
 * = the average of Inside Value and Outside Value), then a Fast Chamfer Distance pass propagates that band outward to
 * fill the whole image. The result is approximate (chamfer-metric) away from the boundary; when an exact signed
 * distance is required, prefer the Signed Maurer Distance Map Image Filter.
 *
 * The input must be a single-component (scalar) integer image; the output is a FIXED float32 image regardless of the
 * input element type. This matches the legacy ITK Approximate Signed Distance Map Image Filter exactly and is
 * out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT ApproximateSignedDistanceMapImageFilter : public IFilter
{
public:
  ApproximateSignedDistanceMapImageFilter() = default;
  ~ApproximateSignedDistanceMapImageFilter() noexcept override = default;

  ApproximateSignedDistanceMapImageFilter(const ApproximateSignedDistanceMapImageFilter&) = delete;
  ApproximateSignedDistanceMapImageFilter(ApproximateSignedDistanceMapImageFilter&&) noexcept = delete;
  ApproximateSignedDistanceMapImageFilter& operator=(const ApproximateSignedDistanceMapImageFilter&) = delete;
  ApproximateSignedDistanceMapImageFilter& operator=(ApproximateSignedDistanceMapImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_InsideValue_Key = "inside_value";
  static constexpr StringLiteral k_OutsideValue_Key = "outside_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ApproximateSignedDistanceMapImageFilter, "e0e0c1b1-909b-4be9-8187-c08472e4b4c3");
