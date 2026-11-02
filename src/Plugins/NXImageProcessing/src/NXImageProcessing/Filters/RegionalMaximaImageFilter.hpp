#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RegionalMaximaImageFilter
 * @brief ITK-free, out-of-core-capable regional maxima: produce a binary image where the foreground marks the
 *        regional maxima of the input and the background marks everything else.
 *
 * A regional maximum is a connected flat zone surrounded entirely by pixels of strictly lower value. This filter
 * writes the Foreground Value at every regional-maximum pixel and the Background Value everywhere else. A
 * completely flat image is treated as a single regional maximum when Flat Is Maxima is On (all foreground),
 * otherwise all background. Output type == input type (the foreground/background values are of the input type); the
 * input must be single-component (scalar). This matches the legacy ITK Regional Maxima Image Filter exactly and is
 * out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT RegionalMaximaImageFilter : public IFilter
{
public:
  RegionalMaximaImageFilter() = default;
  ~RegionalMaximaImageFilter() noexcept override = default;

  RegionalMaximaImageFilter(const RegionalMaximaImageFilter&) = delete;
  RegionalMaximaImageFilter(RegionalMaximaImageFilter&&) noexcept = delete;
  RegionalMaximaImageFilter& operator=(const RegionalMaximaImageFilter&) = delete;
  RegionalMaximaImageFilter& operator=(RegionalMaximaImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_FlatIsMaxima_Key = "flat_is_maxima";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RegionalMaximaImageFilter, "b1437d92-4400-435e-b7dc-d7b8e3e560ad");
