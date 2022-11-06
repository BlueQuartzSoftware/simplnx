#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindEuclideanDistMapFilter
 * @brief This filter will ....
 */
class CORE_EXPORT FindEuclideanDistMapFilter : public IFilter
{
public:
  FindEuclideanDistMapFilter() = default;
  ~FindEuclideanDistMapFilter() noexcept override = default;

  FindEuclideanDistMapFilter(const FindEuclideanDistMapFilter&) = delete;
  FindEuclideanDistMapFilter(FindEuclideanDistMapFilter&&) noexcept = delete;

  FindEuclideanDistMapFilter& operator=(const FindEuclideanDistMapFilter&) = delete;
  FindEuclideanDistMapFilter& operator=(FindEuclideanDistMapFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CalcManhattanDist_Key = "calc_manhattan_dist";
  static inline constexpr StringLiteral k_DoBoundaries_Key = "do_boundaries";
  static inline constexpr StringLiteral k_DoTripleLines_Key = "do_triple_lines";
  static inline constexpr StringLiteral k_DoQuadPoints_Key = "do_quad_points";
  static inline constexpr StringLiteral k_SaveNearestNeighbors_Key = "save_nearest_neighbors";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_GBDistancesArrayName_Key = "g_bdistances_array_name";
  static inline constexpr StringLiteral k_TJDistancesArrayName_Key = "t_jdistances_array_name";
  static inline constexpr StringLiteral k_QPDistancesArrayName_Key = "q_pdistances_array_name";
  static inline constexpr StringLiteral k_NearestNeighborsArrayName_Key = "nearest_neighbors_array_name";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindEuclideanDistMapFilter, "ba9ae8f6-443e-41d3-bb45-a08a139325c1");
