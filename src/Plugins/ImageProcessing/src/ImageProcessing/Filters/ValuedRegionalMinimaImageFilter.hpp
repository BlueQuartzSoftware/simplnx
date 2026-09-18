#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ValuedRegionalMinimaImageFilter
 * @brief ITK-free, out-of-core-capable valued regional minima: every pixel that is NOT part of a regional minimum
 *        is set to the maximum value of the pixel type; pixels that are regional minima retain their value.
 *
 * A regional minimum is a connected flat zone surrounded entirely by pixels of strictly higher value. This filter
 * keeps those zones at their original intensity and sets everything else to the type maximum. Output type == input
 * type; the input must be single-component (scalar). This matches the legacy ITK Valued Regional Minima Image
 * Filter exactly for integer and float types, and is out-of-core capable (in-core flood vs streamed sweep, both
 * producing bit-identical output).
 */
class IMAGEPROCESSING_EXPORT ValuedRegionalMinimaImageFilter : public IFilter
{
public:
  ValuedRegionalMinimaImageFilter() = default;
  ~ValuedRegionalMinimaImageFilter() noexcept override = default;

  ValuedRegionalMinimaImageFilter(const ValuedRegionalMinimaImageFilter&) = delete;
  ValuedRegionalMinimaImageFilter(ValuedRegionalMinimaImageFilter&&) noexcept = delete;
  ValuedRegionalMinimaImageFilter& operator=(const ValuedRegionalMinimaImageFilter&) = delete;
  ValuedRegionalMinimaImageFilter& operator=(ValuedRegionalMinimaImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ValuedRegionalMinimaImageFilter, "c5fe4d67-4171-4982-b35e-5a7411e73c1f");
