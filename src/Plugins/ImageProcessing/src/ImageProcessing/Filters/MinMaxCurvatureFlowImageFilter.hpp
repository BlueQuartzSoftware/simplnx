#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class MinMaxCurvatureFlowImageFilter
 * @brief ITK-free, out-of-core-capable min/max curvature-flow image denoising (a PDE-based finite-difference filter).
 *
 * Iso-brightness contours in the grayscale input image are viewed as a level set, which is evolved by the same
 * curvature-based speed function as CurvatureFlowImageFilter, EXCEPT the update at each voxel is gated on/off by
 * comparing the average image value over a ball-shaped stencil of radius StencilRadius to a threshold sampled along
 * the local gradient direction: F = max(kappa,0) if the stencil average is below the threshold, min(kappa,0)
 * otherwise. This "min/max" switch lets the scale of the noise removed be tuned via StencilRadius, independent of
 * the curvature-flow timestep. Updates are computed with a Jacobi (frozen-neighborhood) finite-difference iteration
 * and a ZeroFluxNeumann (edge-clamp) boundary condition, streamed via the shared FiniteDifferenceEngine. The input
 * must be a floating-point scalar type (float32 or float64); the output has the SAME type as the input. Matches the
 * legacy ITK Min Max Curvature Flow Image Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT MinMaxCurvatureFlowImageFilter : public IFilter
{
public:
  MinMaxCurvatureFlowImageFilter() = default;
  ~MinMaxCurvatureFlowImageFilter() noexcept override = default;

  MinMaxCurvatureFlowImageFilter(const MinMaxCurvatureFlowImageFilter&) = delete;
  MinMaxCurvatureFlowImageFilter(MinMaxCurvatureFlowImageFilter&&) noexcept = delete;
  MinMaxCurvatureFlowImageFilter& operator=(const MinMaxCurvatureFlowImageFilter&) = delete;
  MinMaxCurvatureFlowImageFilter& operator=(MinMaxCurvatureFlowImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_TimeStep_Key = "time_step";
  static constexpr StringLiteral k_NumberOfIterations_Key = "number_of_iterations";
  static constexpr StringLiteral k_StencilRadius_Key = "stencil_radius";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MinMaxCurvatureFlowImageFilter, "142e4792-1174-4290-bac4-c8034ccf29d2");
