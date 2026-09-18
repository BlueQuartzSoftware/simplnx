#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ValuedRegionalMaximaImageFilter
 * @brief ITK-free, out-of-core-capable valued regional maxima: every pixel that is NOT part of a regional maximum
 *        is set to the minimum value of the pixel type; pixels that are regional maxima retain their value.
 *
 * A regional maximum is a connected flat zone surrounded entirely by pixels of strictly lower value. This filter
 * keeps those zones at their original intensity and sets everything else to the type minimum. Output type == input
 * type; the input must be single-component (scalar). This matches the legacy ITK Valued Regional Maxima Image
 * Filter exactly for integer and float types, and is out-of-core capable (in-core flood vs streamed sweep, both
 * producing bit-identical output).
 */
class IMAGEPROCESSING_EXPORT ValuedRegionalMaximaImageFilter : public IFilter
{
public:
  ValuedRegionalMaximaImageFilter() = default;
  ~ValuedRegionalMaximaImageFilter() noexcept override = default;

  ValuedRegionalMaximaImageFilter(const ValuedRegionalMaximaImageFilter&) = delete;
  ValuedRegionalMaximaImageFilter(ValuedRegionalMaximaImageFilter&&) noexcept = delete;
  ValuedRegionalMaximaImageFilter& operator=(const ValuedRegionalMaximaImageFilter&) = delete;
  ValuedRegionalMaximaImageFilter& operator=(ValuedRegionalMaximaImageFilter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ValuedRegionalMaximaImageFilter, "40f84912-15bc-45b5-926b-c9802dc45bea");
