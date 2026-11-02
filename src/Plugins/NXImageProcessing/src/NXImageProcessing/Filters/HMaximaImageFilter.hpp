#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class HMaximaImageFilter
 * @brief ITK-free, out-of-core-capable H-Maxima transform (suppresses regional maxima whose height/contrast is
 *        below a user-supplied Height).
 *
 * The H-Maxima transform removes every regional maximum whose depth (contrast above the surrounding level) is
 * less than @c Height, and lowers each surviving maximum by exactly @c Height, leaving the background otherwise
 * unchanged. It is implemented as reconstruction-by-dilation of a marker image equal to the saturating difference
 * (input - Height) under the input itself as the mask (the shared D3-split morphological reconstruction engine).
 * The saturating marker matches the legacy ITK H Maxima Image Filter exactly, including the clamp (rather than
 * wrap) when (input - Height) underflows the element type's range.
 *
 * Output type == input type; the input must be single-component (scalar). This is out-of-core capable (the legacy
 * filter rejected chunked arrays): reconstruction only copies or clamps existing values, so the result is
 * bit-for-bit identical for integer AND float types and matches the legacy ITK output exactly.
 */
class IMAGEPROCESSING_EXPORT HMaximaImageFilter : public IFilter
{
public:
  HMaximaImageFilter() = default;
  ~HMaximaImageFilter() noexcept override = default;

  HMaximaImageFilter(const HMaximaImageFilter&) = delete;
  HMaximaImageFilter(HMaximaImageFilter&&) noexcept = delete;
  HMaximaImageFilter& operator=(const HMaximaImageFilter&) = delete;
  HMaximaImageFilter& operator=(HMaximaImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Height_Key = "height";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, HMaximaImageFilter, "c912774e-be4a-4bc0-b096-9469bf31b4b7");
