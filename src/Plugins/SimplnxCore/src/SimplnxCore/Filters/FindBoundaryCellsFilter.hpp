#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindBoundaryCellsFilter
 * @brief This filter will determine, for each Cell, the number of neighboring Cells that are owned by a different Feature.
 */
class SIMPLNXCORE_EXPORT FindBoundaryCellsFilter : public IFilter
{
public:
  FindBoundaryCellsFilter() = default;
  ~FindBoundaryCellsFilter() noexcept override = default;

  FindBoundaryCellsFilter(const FindBoundaryCellsFilter&) = delete;
  FindBoundaryCellsFilter(FindBoundaryCellsFilter&&) noexcept = delete;

  FindBoundaryCellsFilter& operator=(const FindBoundaryCellsFilter&) = delete;
  FindBoundaryCellsFilter& operator=(FindBoundaryCellsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_IgnoreFeatureZero_Key = "ignore_feature_zero";
  static inline constexpr StringLiteral k_IncludeVolumeBoundary_Key = "include_volume_boundary";
  static inline constexpr StringLiteral k_GeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static inline constexpr StringLiteral k_BoundaryCellsArrayName_Key = "boundary_cells_array_name";

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
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindBoundaryCellsFilter, "a1dd1c29-9152-4648-836c-3b6967e32600");
/* LEGACY UUID FOR THIS FILTER 8a1106d4-c67f-5e09-a02a-b2e9b99d031e */
