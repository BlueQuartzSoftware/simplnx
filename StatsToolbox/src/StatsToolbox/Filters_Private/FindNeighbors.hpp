#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindNeighbors
 * @brief This filter will ....
 */
class STATSTOOLBOX_EXPORT FindNeighbors : public IFilter
{
public:
  FindNeighbors() = default;
  ~FindNeighbors() noexcept override = default;

  FindNeighbors(const FindNeighbors&) = delete;
  FindNeighbors(FindNeighbors&&) noexcept = delete;

  FindNeighbors& operator=(const FindNeighbors&) = delete;
  FindNeighbors& operator=(FindNeighbors&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_StoreBoundaryCells_Key = "StoreBoundaryCells";
  static inline constexpr StringLiteral k_StoreSurfaceFeatures_Key = "StoreSurfaceFeatures";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "CellFeatureAttributeMatrixPath";
  static inline constexpr StringLiteral k_BoundaryCellsArrayName_Key = "BoundaryCellsArrayName";
  static inline constexpr StringLiteral k_NumNeighborsArrayName_Key = "NumNeighborsArrayName";
  static inline constexpr StringLiteral k_NeighborListArrayName_Key = "NeighborListArrayName";
  static inline constexpr StringLiteral k_SharedSurfaceAreaListArrayName_Key = "SharedSurfaceAreaListArrayName";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayName_Key = "SurfaceFeaturesArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindNeighbors, "7177e88c-c3ab-4169-abe9-1fdaff20e598");
