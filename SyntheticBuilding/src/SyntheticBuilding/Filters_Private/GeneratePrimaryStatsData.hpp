#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class GeneratePrimaryStatsData
 * @brief This filter will ....
 */
class SYNTHETICBUILDING_EXPORT GeneratePrimaryStatsData : public IFilter
{
public:
  GeneratePrimaryStatsData() = default;
  ~GeneratePrimaryStatsData() noexcept override = default;

  GeneratePrimaryStatsData(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData(GeneratePrimaryStatsData&&) noexcept = delete;

  GeneratePrimaryStatsData& operator=(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData& operator=(GeneratePrimaryStatsData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseName_Key = "phase_name";
  static inline constexpr StringLiteral k_CrystalSymmetry_Key = "crystal_symmetry";
  static inline constexpr StringLiteral k_MicroPresetModel_Key = "micro_preset_model";
  static inline constexpr StringLiteral k_PhaseFraction_Key = "phase_fraction";
  static inline constexpr StringLiteral k_Mu_Key = "mu";
  static inline constexpr StringLiteral k_Sigma_Key = "sigma";
  static inline constexpr StringLiteral k_MinCutOff_Key = "min_cut_off";
  static inline constexpr StringLiteral k_MaxCutOff_Key = "max_cut_off";
  static inline constexpr StringLiteral k_BinStepSize_Key = "bin_step_size";
  static inline constexpr StringLiteral k_OdfData_Key = "odf_data";
  static inline constexpr StringLiteral k_MdfData_Key = "mdf_data";
  static inline constexpr StringLiteral k_AxisOdfData_Key = "axis_odf_data";
  static inline constexpr StringLiteral k_CreateEnsembleAttributeMatrix_Key = "create_ensemble_attribute_matrix";
  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "cell_ensemble_attribute_matrix_name";
  static inline constexpr StringLiteral k_AppendToExistingAttributeMatrix_Key = "append_to_existing_attribute_matrix";
  static inline constexpr StringLiteral k_SelectedEnsembleAttributeMatrix_Key = "selected_ensemble_attribute_matrix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, GeneratePrimaryStatsData, "feca9156-280f-4a10-a9da-64cd4558893d");
