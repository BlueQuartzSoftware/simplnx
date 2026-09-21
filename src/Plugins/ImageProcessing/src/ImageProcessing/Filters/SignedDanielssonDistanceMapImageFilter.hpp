#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SignedDanielssonDistanceMapImageFilter
 * @brief ITK-free, out-of-core-capable signed Danielsson distance transform: produce a float32 image whose value at
 *        each voxel is the SIGNED (approximate) Euclidean distance to the object boundary.
 *
 * This reproduces ITK's composite exactly: it runs the Danielsson 4SED distance transform twice -- once on the input
 * and once on the dilated inverted input -- and subtracts the two distance maps. The result is negative inside the
 * object (nonzero voxels) and positive outside; enabling Inside Is Positive flips that sign convention. The distance
 * may optionally be left as a squared distance (Squared Distance) and optionally scaled by the per-axis voxel spacing
 * (Use Image Spacing). Because it is built on Danielsson's 4SED, it is an approximation of the exact signed Euclidean
 * distance (matching the legacy filter exactly); prefer the Signed Maurer Distance Map Image Filter when an exact
 * transform is required. The input must be a single-component (scalar) integer image; the output is a FIXED float32
 * distance image regardless of the input element type.
 */
class IMAGEPROCESSING_EXPORT SignedDanielssonDistanceMapImageFilter : public IFilter
{
public:
  SignedDanielssonDistanceMapImageFilter() = default;
  ~SignedDanielssonDistanceMapImageFilter() noexcept override = default;

  SignedDanielssonDistanceMapImageFilter(const SignedDanielssonDistanceMapImageFilter&) = delete;
  SignedDanielssonDistanceMapImageFilter(SignedDanielssonDistanceMapImageFilter&&) noexcept = delete;
  SignedDanielssonDistanceMapImageFilter& operator=(const SignedDanielssonDistanceMapImageFilter&) = delete;
  SignedDanielssonDistanceMapImageFilter& operator=(SignedDanielssonDistanceMapImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_InsideIsPositive_Key = "inside_is_positive";
  static constexpr StringLiteral k_SquaredDistance_Key = "squared_distance";
  static constexpr StringLiteral k_UseImageSpacing_Key = "use_image_spacing";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SignedDanielssonDistanceMapImageFilter, "a46ca1c9-5be6-4ef8-ad9d-6afab04b4e62");
