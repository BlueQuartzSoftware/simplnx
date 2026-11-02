#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GradientAnisotropicDiffusionImageFilter
 * @brief ITK-free, out-of-core-capable Perona-Malik gradient anisotropic-diffusion image denoising (a PDE-based
 *        finite-difference filter).
 *
 * The classic Perona-Malik anisotropic diffusion equation for scalar-valued images: the conductance term is a
 * function of the local gradient magnitude, C(x) = exp(-(|grad(I)|/K)^2), which reduces the strength of diffusion
 * at edge pixels (where the gradient is large) relative to smooth regions -- unlike plain curvature flow, this
 * preserves edges while smoothing noise elsewhere. K is recalibrated every ConductanceScalingUpdateInterval
 * iterations (and always at the first iteration) from a GLOBAL reduction: the average squared gradient magnitude
 * over the entire image, computed as a single deterministic ordered accumulation so in-core and out-of-core runs
 * are byte-identical. Updates are computed with a Jacobi (frozen-neighborhood) finite-difference iteration and a
 * ZeroFluxNeumann (edge-clamp) boundary condition, streamed via the shared FiniteDifferenceEngine. The input must
 * be a floating-point scalar type (float32 or float64); the output has the SAME type as the input. A Time Step
 * that exceeds the CFL stability bound (minSpacing / 2^(ImageDimension+1)) produces a preflight WARNING (never a
 * hard rejection), matching ITK's own non-fatal check. Matches the legacy ITK Gradient Anisotropic Diffusion Image
 * Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT GradientAnisotropicDiffusionImageFilter : public IFilter
{
public:
  GradientAnisotropicDiffusionImageFilter() = default;
  ~GradientAnisotropicDiffusionImageFilter() noexcept override = default;

  GradientAnisotropicDiffusionImageFilter(const GradientAnisotropicDiffusionImageFilter&) = delete;
  GradientAnisotropicDiffusionImageFilter(GradientAnisotropicDiffusionImageFilter&&) noexcept = delete;
  GradientAnisotropicDiffusionImageFilter& operator=(const GradientAnisotropicDiffusionImageFilter&) = delete;
  GradientAnisotropicDiffusionImageFilter& operator=(GradientAnisotropicDiffusionImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_TimeStep_Key = "time_step";
  static constexpr StringLiteral k_ConductanceParameter_Key = "conductance_parameter";
  static constexpr StringLiteral k_ConductanceScalingUpdateInterval_Key = "conductance_scaling_update_interval";
  static constexpr StringLiteral k_NumberOfIterations_Key = "number_of_iterations";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GradientAnisotropicDiffusionImageFilter, "17d499f1-b2dd-400d-a791-4ed9ea6903c0");
