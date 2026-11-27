#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class HConvexImageFilter
 * @brief ITK-free, out-of-core-capable h-convex transform: each regional maximum is replaced by its own height,
 *        capped at the Height parameter (its "dome"), and everything else becomes zero.
 *
 * Reconstructs the input minus Height by dilation under the input mask, then subtracts the reconstruction from the input.
 * The output has the input's scalar type and shape. Height must be finite and nonnegative. Integer images require a whole
 * Height and use native marker subtraction that saturates at the type minimum. Height 0 gives zero integer contrast.
 * Height uses float64 precision, which cannot distinguish every consecutive integer above 2^53. Floating images allow fractional Height.
 * An execution error identifies the first tuple where contrast exceeds the signed output type's maximum.
 */
class NXIMAGEPROCESSING_EXPORT HConvexImageFilter : public IFilter
{
public:
  HConvexImageFilter() = default;
  ~HConvexImageFilter() noexcept override = default;

  HConvexImageFilter(const HConvexImageFilter&) = delete;
  HConvexImageFilter(HConvexImageFilter&&) noexcept = delete;
  HConvexImageFilter& operator=(const HConvexImageFilter&) = delete;
  HConvexImageFilter& operator=(HConvexImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteralType k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteralType k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteralType k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteralType k_Height_Key = "height";
  static constexpr StringLiteralType k_FullyConnected_Key = "fully_connected";

  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointerType clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, HConvexImageFilter, "a11f18b6-87df-41a4-9fd7-87050ecf147e");
