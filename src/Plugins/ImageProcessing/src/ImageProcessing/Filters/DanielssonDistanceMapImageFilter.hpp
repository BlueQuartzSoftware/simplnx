#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class DanielssonDistanceMapImageFilter
 * @brief ITK-free, out-of-core-capable Danielsson distance transform: produce a float32 image whose value at each
 *        voxel is the (approximate) Euclidean distance to the nearest foreground voxel.
 *
 * Following ITK's Danielsson convention, NONZERO input voxels are the foreground "features" (distance 0) and ZERO
 * voxels are solved -- each stores the distance to its nearest foreground voxel via a 4SED vector propagation. The
 * distance may optionally be left as a squared distance (Squared Distance) and optionally scaled by the per-axis voxel
 * spacing (Use Image Spacing). Input Is Binary is accepted for parity with the legacy filter but only affects the
 * (unemitted) Voronoi label map, never the distance output. The input must be a single-component (scalar) integer
 * image; the output is a FIXED float32 distance image regardless of the input element type. This matches the legacy
 * ITK Danielsson Distance Map Image Filter exactly and is out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT DanielssonDistanceMapImageFilter : public IFilter
{
public:
  DanielssonDistanceMapImageFilter() = default;
  ~DanielssonDistanceMapImageFilter() noexcept override = default;

  DanielssonDistanceMapImageFilter(const DanielssonDistanceMapImageFilter&) = delete;
  DanielssonDistanceMapImageFilter(DanielssonDistanceMapImageFilter&&) noexcept = delete;
  DanielssonDistanceMapImageFilter& operator=(const DanielssonDistanceMapImageFilter&) = delete;
  DanielssonDistanceMapImageFilter& operator=(DanielssonDistanceMapImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_InputIsBinary_Key = "input_is_binary";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, DanielssonDistanceMapImageFilter, "f044feae-968b-4212-8898-e26e305afa0b");
