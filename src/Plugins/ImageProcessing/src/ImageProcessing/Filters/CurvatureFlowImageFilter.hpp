#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class CurvatureFlowImageFilter
 * @brief ITK-free, out-of-core-capable curvature-driven image denoising (a PDE-based finite-difference filter).
 *
 * Iso-brightness contours in the grayscale input image are viewed as a level set, which is evolved by the
 * curvature-based speed function I_t = kappa*|grad(I)| for NumberOfIterations, using a fixed TimeStep. Updates are
 * computed with a Jacobi (frozen-neighborhood) finite-difference iteration and a ZeroFluxNeumann (edge-clamp)
 * boundary condition, streamed via the shared FiniteDifferenceEngine. The input may be ANY scalar type (integer OR
 * floating point); the output has the SAME type as the input (integer output truncates on every iteration's
 * ApplyUpdate, matching ITK). Matches the legacy ITK Curvature Flow Image Filter. Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT CurvatureFlowImageFilter : public IFilter
{
public:
  CurvatureFlowImageFilter() = default;
  ~CurvatureFlowImageFilter() noexcept override = default;

  CurvatureFlowImageFilter(const CurvatureFlowImageFilter&) = delete;
  CurvatureFlowImageFilter(CurvatureFlowImageFilter&&) noexcept = delete;
  CurvatureFlowImageFilter& operator=(const CurvatureFlowImageFilter&) = delete;
  CurvatureFlowImageFilter& operator=(CurvatureFlowImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_TimeStep_Key = "time_step";
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CurvatureFlowImageFilter, "63668df1-f887-4d39-9b82-e8a0963fbf52");
