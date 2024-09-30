#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class NeighborOrientationCorrelationFilter
 * @brief This filter will ....
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

  // Parameter Keys
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_MinConfidence_Key = "min_confidence";
  static inline constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  static inline constexpr StringLiteral k_Level_Key = "level";
  static inline constexpr StringLiteral k_CorrelationArrayPath_Key = "correlation_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";

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
   * @brief Returns the human readable name of the filter.
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
   * Initial version should always be 1.
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
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, NeighborOrientationCorrelationFilter, "4625c192-7e46-4333-a294-67a2eb64cb37");
