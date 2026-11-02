#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RegionalMinimaImageFilter
 * @brief ITK-free, out-of-core-capable regional minima: produce a binary image where the foreground marks the
 *        regional minima of the input and the background marks everything else.
 *
 * A regional minimum is a connected flat zone surrounded entirely by pixels of strictly higher value. This filter
 * writes the Foreground Value at every regional-minimum pixel and the Background Value everywhere else. A
 * completely flat image is treated as a single regional minimum when Flat Is Minima is On (all foreground),
 * otherwise all background. Output type == input type (the foreground/background values are of the input type); the
 * input must be single-component (scalar). This matches the legacy ITK Regional Minima Image Filter exactly and is
 * out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT RegionalMinimaImageFilter : public IFilter
{
public:
  RegionalMinimaImageFilter() = default;
  ~RegionalMinimaImageFilter() noexcept override = default;

  RegionalMinimaImageFilter(const RegionalMinimaImageFilter&) = delete;
  RegionalMinimaImageFilter(RegionalMinimaImageFilter&&) noexcept = delete;
  RegionalMinimaImageFilter& operator=(const RegionalMinimaImageFilter&) = delete;
  RegionalMinimaImageFilter& operator=(RegionalMinimaImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_ForegroundValue_Key = "foreground_value";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_FlatIsMinima_Key = "flat_is_minima";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RegionalMinimaImageFilter, "38b0d575-d728-49e9-9611-42c7df83c96c");
