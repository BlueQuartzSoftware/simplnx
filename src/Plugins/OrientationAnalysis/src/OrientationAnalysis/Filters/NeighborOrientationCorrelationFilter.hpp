#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class NeighborOrientationCorrelationFilter
 * @brief Cleans up EBSD data by replacing low-confidence voxels with data from
 * the most orientation-correlated face neighbor.
 *
 * The filter compares the six face neighbors of each low-confidence voxel.
 * It selects the neighbor that agrees with the most other neighbors within the
 * misorientation tolerance. It then copies that neighbor across non-ignored
 * cell arrays.
 *
 * The algorithm uses a rolling three-slice window and bulk store operations.
 * This gives backend-independent sequential access for out-of-core arrays.
 * Cancellation returns success with completed slice updates preserved.
 */
class ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelationFilter : public IFilter
{
public:
  NeighborOrientationCorrelationFilter() = default;
  ~NeighborOrientationCorrelationFilter() noexcept override = default;

  NeighborOrientationCorrelationFilter(const NeighborOrientationCorrelationFilter&) = delete;
  NeighborOrientationCorrelationFilter(NeighborOrientationCorrelationFilter&&) noexcept = delete;

  NeighborOrientationCorrelationFilter& operator=(const NeighborOrientationCorrelationFilter&) = delete;
  NeighborOrientationCorrelationFilter& operator=(NeighborOrientationCorrelationFilter&&) noexcept = delete;

  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_MinConfidence_Key = "min_confidence";
  static constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  static constexpr StringLiteral k_Level_Key = "level";
  static constexpr StringLiteral k_CorrelationArrayPath_Key = "correlation_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";

  /**
   * @brief Converts a legacy SIMPL parameter object.
   * @param json Legacy filter parameters.
   * @return Converted simplnx arguments or conversion errors.
   */
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, NeighborOrientationCorrelationFilter, "4625c192-7e46-4333-a294-67a2eb64cb37");
