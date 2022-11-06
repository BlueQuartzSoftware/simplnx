#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindLayerStatistics
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT FindLayerStatistics : public IFilter
{
public:
  FindLayerStatistics() = default;
  ~FindLayerStatistics() noexcept override = default;

  FindLayerStatistics(const FindLayerStatistics&) = delete;
  FindLayerStatistics(FindLayerStatistics&&) noexcept = delete;

  FindLayerStatistics& operator=(const FindLayerStatistics&) = delete;
  FindLayerStatistics& operator=(FindLayerStatistics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Plane_Key = "plane";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "selected_array_path";
  static inline constexpr StringLiteral k_LayerAttributeMatrixName_Key = "layer_attribute_matrix_name";
  static inline constexpr StringLiteral k_LayerIDsArrayName_Key = "layer_ids_array_name";
  static inline constexpr StringLiteral k_LayerMinArrayName_Key = "layer_min_array_name";
  static inline constexpr StringLiteral k_LayerMaxArrayName_Key = "layer_max_array_name";
  static inline constexpr StringLiteral k_LayerAvgArrayName_Key = "layer_avg_array_name";
  static inline constexpr StringLiteral k_LayerStdArrayName_Key = "layer_std_array_name";
  static inline constexpr StringLiteral k_LayerVarArrayName_Key = "layer_var_array_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindLayerStatistics, "4d02d8db-7686-4acd-b468-e5f05805ce76");
