#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ResampleImageGeomFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ResampleImageGeomFilter : public IFilter
{
public:
  ResampleImageGeomFilter() = default;
  ~ResampleImageGeomFilter() noexcept override = default;

  ResampleImageGeomFilter(const ResampleImageGeomFilter&) = delete;
  ResampleImageGeomFilter(ResampleImageGeomFilter&&) noexcept = delete;

  ResampleImageGeomFilter& operator=(const ResampleImageGeomFilter&) = delete;
  ResampleImageGeomFilter& operator=(ResampleImageGeomFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ResamplingMode_Key = "resampling_mode";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";
  static inline constexpr StringLiteral k_Scaling_Key = "scaling";
  static inline constexpr StringLiteral k_ExactDimensions_Key = "exact_dimensions";
  static inline constexpr StringLiteral k_RenumberFeatures_Key = "renumber_features";
  static inline constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeatureAttributeMatrix_Key = "cell_feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_CreatedImageGeometry_Key = "new_data_container_path";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";

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
   * @param data The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param data The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ResampleImageGeomFilter, "9783ea2c-4cf7-46de-ab21-b40d91a48c5b");
