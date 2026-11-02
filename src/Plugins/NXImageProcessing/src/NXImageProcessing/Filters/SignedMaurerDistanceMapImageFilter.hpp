#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class SignedMaurerDistanceMapImageFilter
 * @brief ITK-free, out-of-core-capable signed Maurer distance transform: produce a float32 image whose value at each
 *        voxel is the signed Euclidean distance to the boundary of the object defined by the input image.
 *
 * A voxel is considered "inside" the object iff its value is not the Background Value. By default the distance is
 * negative inside the object and positive outside; enabling Inside Is Positive flips that sign convention. The
 * distance may optionally be left as a squared distance (Squared Distance, the default) and optionally scaled by the
 * per-axis voxel spacing (Use Image Spacing). The input must be a single-component (scalar) integer image; the output
 * is a FIXED float32 distance image regardless of the input element type. This matches the legacy ITK Signed Maurer
 * Distance Map Image Filter exactly and is out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT SignedMaurerDistanceMapImageFilter : public IFilter
{
public:
  SignedMaurerDistanceMapImageFilter() = default;
  ~SignedMaurerDistanceMapImageFilter() noexcept override = default;

  SignedMaurerDistanceMapImageFilter(const SignedMaurerDistanceMapImageFilter&) = delete;
  SignedMaurerDistanceMapImageFilter(SignedMaurerDistanceMapImageFilter&&) noexcept = delete;
  SignedMaurerDistanceMapImageFilter& operator=(const SignedMaurerDistanceMapImageFilter&) = delete;
  SignedMaurerDistanceMapImageFilter& operator=(SignedMaurerDistanceMapImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_InsideIsPositive_Key = "inside_is_positive";
  static constexpr StringLiteral k_SquaredDistance_Key = "squared_distance";
  static constexpr StringLiteral k_UseImageSpacing_Key = "use_image_spacing";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, SignedMaurerDistanceMapImageFilter, "7958cf68-fb34-4616-aee7-35ddfc0e49f2");
