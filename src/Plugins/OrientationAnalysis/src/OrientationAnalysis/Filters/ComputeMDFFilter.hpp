#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeMDFFilter
 * @brief Computes the correlated Misorientation Distribution Function (MDF) for each Ensemble/Phase
 * from per-Feature boundary misorientations using a de la Vallee Poussin kernel density estimate.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeMDFFilter : public IFilter
{
public:
  ComputeMDFFilter() = default;
  ~ComputeMDFFilter() noexcept override = default;

  ComputeMDFFilter(const ComputeMDFFilter&) = delete;
  ComputeMDFFilter(ComputeMDFFilter&&) noexcept = delete;

  ComputeMDFFilter& operator=(const ComputeMDFFilter&) = delete;
  ComputeMDFFilter& operator=(ComputeMDFFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static constexpr StringLiteral k_Halfwidth_Key = "kernel_halfwidth_degrees";
  static constexpr StringLiteral k_NumCurvePoints_Key = "number_of_curve_points";
  static constexpr StringLiteral k_UseAreaWeights_Key = "weight_by_boundary_area";
  static constexpr StringLiteral k_OutputGroupPath_Key = "output_group_path";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeMDFFilter, "549c9da7-b891-4926-88a2-1c37e19bf182");
