#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class HMinimaImageFilter
 * @brief Suppresses shallow regional minima by reconstructing the raised input marker by erosion.
 *
 * The marker is input plus Height, and the original input is the lower mask for reconstruction.
 * Integer images require finite, nonnegative whole Height values. Integer marker addition uses native values and saturates at the type maximum.
 * Height 0 preserves integer input exactly, including native 64-bit units. Floating images retain fractional Height and existing floating arithmetic.
 * Height uses float64 precision, which cannot distinguish every consecutive integer above 2^53.
 *
 * Input must have one component per tuple. Output contains reconstructed intensities with the same type and shape as input.
 * Resident and disk-backed reconstruction retain their existing storage routes. Corrected integer markers can change downstream minima and watershed seeds.
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
