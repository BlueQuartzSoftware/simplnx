#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeKernelAvgMisorientationsFilter
 * @brief Computes Kernel Average Misorientation for each cell.
 *
 * The filter averages kernel neighbors within each feature by default. The
 * Use Feature Ids option permits averaging across feature boundaries.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsFilter : public IFilter
{
public:
  ComputeKernelAvgMisorientationsFilter() = default;
  ~ComputeKernelAvgMisorientationsFilter() noexcept override = default;

  ComputeKernelAvgMisorientationsFilter(const ComputeKernelAvgMisorientationsFilter&) = delete;
  ComputeKernelAvgMisorientationsFilter(ComputeKernelAvgMisorientationsFilter&&) noexcept = delete;

  ComputeKernelAvgMisorientationsFilter& operator=(const ComputeKernelAvgMisorientationsFilter&) = delete;
  ComputeKernelAvgMisorientationsFilter& operator=(ComputeKernelAvgMisorientationsFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_KernelSize_Key = "kernel_size";
  static constexpr StringLiteral k_UseFeatureIds_Key = "use_feature_ids";
  static constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static constexpr StringLiteral k_KernelAverageMisorientationsArrayName_Key = "kernel_average_misorientations_array_name";
  static constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
   * @brief Validates arguments and prepares actions without changing the DataStructure.
   * @param dataStructure Input DataStructure.
   * @param filterArgs Filter parameter values.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Resolves relative paths.
   * @return Preflight actions, warnings, and errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Computes KAM values for the selected geometry.
   * @param dataStructure DataStructure to update.
   * @param filterArgs Filter parameter values.
   * @param pipelineNode Optional pipeline node.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Resolves relative paths.
   * @return Execution warnings and errors. A failure can leave partial output.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeKernelAvgMisorientationsFilter, "61cfc9c1-aa0e-452b-b9ef-d3b9e6268035");
