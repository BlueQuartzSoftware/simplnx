#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GrayscaleFillholeImageFilter
 * @brief ITK-free, out-of-core-capable grayscale fillhole (removes dark holes / regional minima not connected
 *        to the image border).
 *
 * A "hole" is a connected set of interior voxels darker than the surrounding level -- a regional minimum in the
 * grayscale topography that is not connected to the boundary of the image. This filter raises each such minimum
 * up to the level of the darkest surrounding path to the border, leaving every local maximum untouched.
 *
 * It is implemented as reconstruction-by-erosion of a marker image whose border voxels equal the input and whose
 * interior is the input's global maximum, using the input itself as the mask (the shared D3-split morphological
 * reconstruction engine). Output type == input type; the input must be single-component (scalar). This matches
 * the legacy ITK Grayscale Fillhole Image Filter exactly, and is out-of-core capable (the legacy filter rejected
 * chunked arrays): reconstruction only copies or clamps existing input values, so the result is bit-for-bit
 * identical for integer AND float types.
 */
class IMAGEPROCESSING_EXPORT GrayscaleFillholeImageFilter : public IFilter
{
public:
  GrayscaleFillholeImageFilter() = default;
  ~GrayscaleFillholeImageFilter() noexcept override = default;

  GrayscaleFillholeImageFilter(const GrayscaleFillholeImageFilter&) = delete;
  GrayscaleFillholeImageFilter(GrayscaleFillholeImageFilter&&) noexcept = delete;
  GrayscaleFillholeImageFilter& operator=(const GrayscaleFillholeImageFilter&) = delete;
  GrayscaleFillholeImageFilter& operator=(GrayscaleFillholeImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GrayscaleFillholeImageFilter, "aa11f464-039e-4e7b-b748-18cc72b3f054");
