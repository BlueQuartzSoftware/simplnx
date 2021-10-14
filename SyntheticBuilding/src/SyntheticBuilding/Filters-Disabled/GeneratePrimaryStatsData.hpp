#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class GeneratePrimaryStatsData
 * @brief This filter will ....
 */
class COMPLEX_EXPORT GeneratePrimaryStatsData : public IFilter
{
public:
  GeneratePrimaryStatsData() = default;
  ~GeneratePrimaryStatsData() noexcept override = default;

  GeneratePrimaryStatsData(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData(GeneratePrimaryStatsData&&) noexcept = delete;

  GeneratePrimaryStatsData& operator=(const GeneratePrimaryStatsData&) = delete;
  GeneratePrimaryStatsData& operator=(GeneratePrimaryStatsData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseName_Key = "PhaseName";
  static inline constexpr StringLiteral k_CrystalSymmetry_Key = "CrystalSymmetry";
  static inline constexpr StringLiteral k_MicroPresetModel_Key = "MicroPresetModel";
  static inline constexpr StringLiteral k_PhaseFraction_Key = "PhaseFraction";
  static inline constexpr StringLiteral k_Mu_Key = "Mu";
  static inline constexpr StringLiteral k_Sigma_Key = "Sigma";
  static inline constexpr StringLiteral k_MinCutOff_Key = "MinCutOff";
  static inline constexpr StringLiteral k_MaxCutOff_Key = "MaxCutOff";
  static inline constexpr StringLiteral k_BinStepSize_Key = "BinStepSize";
  static inline constexpr StringLiteral k_NumberOfBins_Key = "NumberOfBins";
  static inline constexpr StringLiteral k_FeatureESD_Key = "FeatureESD";
  static inline constexpr StringLiteral k_OdfData_Key = "OdfData";
  static inline constexpr StringLiteral k_MdfData_Key = "MdfData";
  static inline constexpr StringLiteral k_AxisOdfData_Key = "AxisOdfData";
  static inline constexpr StringLiteral k_CreateEnsembleAttributeMatrix_Key = "CreateEnsembleAttributeMatrix";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "CellEnsembleAttributeMatrixName";
  static inline constexpr StringLiteral k_AppendToExistingAttributeMatrix_Key = "AppendToExistingAttributeMatrix";
  static inline constexpr StringLiteral k_SelectedEnsembleAttributeMatrix_Key = "SelectedEnsembleAttributeMatrix";

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
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, GeneratePrimaryStatsData, "b28ca241-0fe6-5353-8a2a-4f8ce468b82a");
