#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ImportH5OimData
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT ImportH5OimData : public IFilter
{
public:
  ImportH5OimData() = default;
  ~ImportH5OimData() noexcept override = default;

  ImportH5OimData(const ImportH5OimData&) = delete;
  ImportH5OimData(ImportH5OimData&&) noexcept = delete;

  ImportH5OimData& operator=(const ImportH5OimData&) = delete;
  ImportH5OimData& operator=(ImportH5OimData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "InputFile";
  static inline constexpr StringLiteral k_SelectedScanNames_Key = "SelectedScanNames";
  static inline constexpr StringLiteral k_ZSpacing_Key = "ZSpacing";
  static inline constexpr StringLiteral k_Origin_Key = "Origin";
  static inline constexpr StringLiteral k_ReadPatternData_Key = "ReadPatternData";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "CellAttributeMatrixName";
  static inline constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "CellEnsembleAttributeMatrixName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ImportH5OimData, "0bffdd87-4aaf-573b-8bfa-3a004772cccb");
