#pragma once

#include "Reconstruction/Reconstruction_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MergeTwins
 * @brief This filter will ....
 */
class RECONSTRUCTION_EXPORT MergeTwins : public IFilter
{
public:
  MergeTwins() = default;
  ~MergeTwins() noexcept override = default;

  MergeTwins(const MergeTwins&) = delete;
  MergeTwins(MergeTwins&&) noexcept = delete;

  MergeTwins& operator=(const MergeTwins&) = delete;
  MergeTwins& operator=(MergeTwins&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseNonContiguousNeighbors_Key = "UseNonContiguousNeighbors";
  static inline constexpr StringLiteral k_NonContiguousNeighborListArrayPath_Key = "NonContiguousNeighborListArrayPath";
  static inline constexpr StringLiteral k_ContiguousNeighborListArrayPath_Key = "ContiguousNeighborListArrayPath";
  static inline constexpr StringLiteral k_AxisTolerance_Key = "AxisTolerance";
  static inline constexpr StringLiteral k_AngleTolerance_Key = "AngleTolerance";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "AvgQuatsArrayPath";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_CellParentIdsArrayName_Key = "CellParentIdsArrayName";
  static inline constexpr StringLiteral k_NewCellFeatureAttributeMatrixName_Key = "NewCellFeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "FeatureParentIdsArrayName";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "ActiveArrayName";

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MergeTwins, "c9af506e-9ea1-5ff5-a882-fa561def5f52");
