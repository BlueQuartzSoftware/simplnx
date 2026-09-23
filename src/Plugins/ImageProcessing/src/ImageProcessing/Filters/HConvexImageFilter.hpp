#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class HConvexImageFilter
 * @brief ITK-free, out-of-core-capable h-convex transform: each regional maximum is replaced by its own height,
 *        capped at the Height parameter (its "dome"), and everything else becomes zero.
 *
 * HConvex(f, h) = f - HMaxima(f, h). A regional maximum of contrast c maps to min(c, h); the background maps to 0.
 * It is implemented by reconstruction-by-dilation of the (input - Height) marker under the input mask (the HMaxima
 * result) followed by an elementwise subtraction of that result from the input. Output type == input type; the
 * input must be single-component (scalar). This matches the legacy ITK H Convex Image Filter exactly for integer
 * and float types, and is out-of-core capable: the reconstruction result never exceeds the input, so the
 * difference never underflows.
 */
class IMAGEPROCESSING_EXPORT HConvexImageFilter : public IFilter
{
public:
  HConvexImageFilter() = default;
  ~HConvexImageFilter() noexcept override = default;

  HConvexImageFilter(const HConvexImageFilter&) = delete;
  HConvexImageFilter(HConvexImageFilter&&) noexcept = delete;
  HConvexImageFilter& operator=(const HConvexImageFilter&) = delete;
  HConvexImageFilter& operator=(HConvexImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Height_Key = "height";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, HConvexImageFilter, "a11f18b6-87df-41a4-9fd7-87050ecf147e");
