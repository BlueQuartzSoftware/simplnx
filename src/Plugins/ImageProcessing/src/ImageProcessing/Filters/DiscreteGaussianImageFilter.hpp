#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class DiscreteGaussianImageFilter
 * @brief ITK-free, out-of-core-capable separable discrete-Gaussian (FIR) convolution.
 *
 * Convolves the input with a discrete Gaussian along each axis, using a verbatim port of ITK's GaussianOperator
 * modified-Bessel kernel (per-axis variance, truncated by MaximumError and capped at MaximumKernelWidth), cascaded
 * axis-by-axis with a zero-flux (Neumann) boundary. The input may be ANY scalar numeric type (integer OR floating
 * point); the output has the SAME type as the input (integer intermediates truncate between passes, matching ITK).
 * When Use Image Spacing is enabled the per-axis variance is divided by the axis spacing squared. Matches the legacy
 * ITK Discrete Gaussian Image Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT DiscreteGaussianImageFilter : public IFilter
{
public:
  DiscreteGaussianImageFilter() = default;
  ~DiscreteGaussianImageFilter() noexcept override = default;

  DiscreteGaussianImageFilter(const DiscreteGaussianImageFilter&) = delete;
  DiscreteGaussianImageFilter(DiscreteGaussianImageFilter&&) noexcept = delete;
  DiscreteGaussianImageFilter& operator=(const DiscreteGaussianImageFilter&) = delete;
  DiscreteGaussianImageFilter& operator=(DiscreteGaussianImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_Variance_Key = "variance";
  static constexpr StringLiteral k_MaximumKernelWidth_Key = "maximum_kernel_width";
  static constexpr StringLiteral k_MaximumError_Key = "maximum_error";
  static constexpr StringLiteral k_UseImageSpacing_Key = "use_image_spacing";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, DiscreteGaussianImageFilter, "2ddd8df9-a4d8-44e0-94b3-0776c1cdf096");
