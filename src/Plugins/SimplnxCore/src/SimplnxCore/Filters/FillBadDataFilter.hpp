#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FillBadDataFilter
 * @brief This filter will replace data using data from surrounding voxels. See more
 * at the help file.
 */
class SIMPLNXCORE_EXPORT FillBadDataFilter : public IFilter
{
public:
  FillBadDataFilter() = default;
  ~FillBadDataFilter() noexcept override = default;

  FillBadDataFilter(const FillBadDataFilter&) = delete;
  FillBadDataFilter(FillBadDataFilter&&) noexcept = delete;

  FillBadDataFilter& operator=(const FillBadDataFilter&) = delete;
  FillBadDataFilter& operator=(FillBadDataFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MinAllowedDefectSize_Key = "min_allowed_defect_size";
  static inline constexpr StringLiteral k_StoreAsNewPhase_Key = "store_as_new_phase";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_SelectedCellDataGroup_Key = "selected_cell_data_group";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FillBadDataFilter, "a59eb864-9e6b-40bb-9292-e5281b0b4f3e");
