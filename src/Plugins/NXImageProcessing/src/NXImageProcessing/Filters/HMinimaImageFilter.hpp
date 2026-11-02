#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class HMinimaImageFilter
 * @brief ITK-free, out-of-core-capable H-Minima transform (suppresses regional minima whose depth/contrast is
 *        below a user-supplied Height).
 *
 * The H-Minima transform removes every regional minimum whose depth (contrast below the surrounding level) is
 * less than @c Height, and raises each surviving minimum by exactly @c Height, leaving the background otherwise
 * unchanged. It is implemented as reconstruction-by-erosion of a marker image equal to the saturating sum
 * (input + Height) under the input itself as the mask (the shared D3-split morphological reconstruction engine).
 * The saturating marker matches the legacy ITK H Minima Image Filter exactly, including the clamp (rather than
 * wrap) when (input + Height) overflows the element type's range.
 *
 * Output type == input type; the input must be single-component (scalar). This is out-of-core capable (the legacy
 * filter rejected chunked arrays): reconstruction only copies or clamps existing values, so the result is
 * bit-for-bit identical for integer AND float types and matches the legacy ITK output exactly.
 */
class IMAGEPROCESSING_EXPORT HMinimaImageFilter : public IFilter
{
public:
  HMinimaImageFilter() = default;
  ~HMinimaImageFilter() noexcept override = default;

  HMinimaImageFilter(const HMinimaImageFilter&) = delete;
  HMinimaImageFilter(HMinimaImageFilter&&) noexcept = delete;
  HMinimaImageFilter& operator=(const HMinimaImageFilter&) = delete;
  HMinimaImageFilter& operator=(HMinimaImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, HMinimaImageFilter, "2fea5169-5fa2-495f-87d7-e1dc5a73b890");
