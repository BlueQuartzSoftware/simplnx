#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindEuclideanDistMap
 * @brief This filter will ....
 */
class STATSTOOLBOX_EXPORT FindEuclideanDistMap : public IFilter
{
public:
  FindEuclideanDistMap() = default;
  ~FindEuclideanDistMap() noexcept override = default;

  FindEuclideanDistMap(const FindEuclideanDistMap&) = delete;
  FindEuclideanDistMap(FindEuclideanDistMap&&) noexcept = delete;

  FindEuclideanDistMap& operator=(const FindEuclideanDistMap&) = delete;
  FindEuclideanDistMap& operator=(FindEuclideanDistMap&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CalcManhattanDist_Key = "CalcManhattanDist";
  static inline constexpr StringLiteral k_DoBoundaries_Key = "DoBoundaries";
  static inline constexpr StringLiteral k_DoTripleLines_Key = "DoTripleLines";
  static inline constexpr StringLiteral k_DoQuadPoints_Key = "DoQuadPoints";
  static inline constexpr StringLiteral k_SaveNearestNeighbors_Key = "SaveNearestNeighbors";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_GBDistancesArrayName_Key = "GBDistancesArrayName";
  static inline constexpr StringLiteral k_TJDistancesArrayName_Key = "TJDistancesArrayName";
  static inline constexpr StringLiteral k_QPDistancesArrayName_Key = "QPDistancesArrayName";
  static inline constexpr StringLiteral k_NearestNeighborsArrayName_Key = "NearestNeighborsArrayName";

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
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindEuclideanDistMap, "933e4b2d-dd61-51c3-98be-00548ba783a3");
