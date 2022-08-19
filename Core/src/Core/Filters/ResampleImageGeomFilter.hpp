#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ResampleImageGeomFilter
 * @brief This filter will ....
 */
class CORE_EXPORT ResampleImageGeomFilter : public IFilter
{
public:
  ResampleImageGeomFilter() = default;
  ~ResampleImageGeomFilter() noexcept override = default;

  ResampleImageGeomFilter(const ResampleImageGeomFilter&) = delete;
  ResampleImageGeomFilter(ResampleImageGeomFilter&&) noexcept = delete;

  ResampleImageGeomFilter& operator=(const ResampleImageGeomFilter&) = delete;
  ResampleImageGeomFilter& operator=(ResampleImageGeomFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Spacing_Key = "Spacing";
  static inline constexpr StringLiteral k_RenumberFeatures_Key = "RenumberFeatures";
  static inline constexpr StringLiteral k_RemoveOriginalGeometry_Key = "RemoveOriginalGeometry";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "CellFeatureAttributeMatrixPath";
  static inline constexpr StringLiteral k_NewDataContainerPath_Key = "NewDataContainerPath";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
  static inline constexpr StringLiteral k_SelectedCellDataGroup_Key = "selected_cell_data_group";
  static inline constexpr StringLiteral k_NewFeaturesName_Key = "new_features_group_name";

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ResampleImageGeomFilter, "9783ea2c-4cf7-46de-ab21-b40d91a48c5b");
