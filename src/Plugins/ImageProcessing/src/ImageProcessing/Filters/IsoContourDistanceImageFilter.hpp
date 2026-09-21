#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class IsoContourDistanceImageFilter
 * @brief ITK-free, out-of-core-capable narrow-band signed distance to a level-set iso-contour.
 *
 * Produces a float32 image whose value at each voxel is the sub-pixel signed distance to the Level Set Value
 * iso-contour of the input, for voxels adjacent to a crossing; voxels away from the contour are set to +Far Value
 * (input above the level set) or -Far Value (below). This is the classic initializer for level-set / fast-marching
 * methods. The transform is Danielsson-free -- it is a single neighborhood pass using gradient interpolation -- and
 * matches the legacy ITK Iso Contour Distance Image Filter exactly.
 *
 * The input may be ANY single-component (scalar) numeric type, integer OR floating point; the output is a FIXED
 * float32 image regardless of the input element type. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT IsoContourDistanceImageFilter : public IFilter
{
public:
  IsoContourDistanceImageFilter() = default;
  ~IsoContourDistanceImageFilter() noexcept override = default;

  IsoContourDistanceImageFilter(const IsoContourDistanceImageFilter&) = delete;
  IsoContourDistanceImageFilter(IsoContourDistanceImageFilter&&) noexcept = delete;
  IsoContourDistanceImageFilter& operator=(const IsoContourDistanceImageFilter&) = delete;
  IsoContourDistanceImageFilter& operator=(IsoContourDistanceImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_LevelSetValue_Key = "level_set_value";
  static constexpr StringLiteral k_FarValue_Key = "far_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, IsoContourDistanceImageFilter, "d0ea04e3-c514-477b-8166-a7998600fea5");
