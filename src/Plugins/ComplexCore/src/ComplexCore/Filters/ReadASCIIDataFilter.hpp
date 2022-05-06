#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

class AbstractDataParser;

namespace complex
{
/**
 * @class ReadASCIIDataFilter
 * @brief This filter reads ASCII data from any text-based file and imports the data into complex-style arrays.
 * The user uses the filter's wizard to specify which file to import, how the data is formatted, what to call
 * each array, and what type each array should be.
 *
 * Note:* This filter is intended to read data that is column-oriented, such that each created complex array
 * corresponds to a column of data in the ASCII file. Therefore, this filter will only import scalar arrays.
 * If multiple columns are in fact different components of the same array, then the columns may be imported as
 * separate arrays and then combined in the correct order using the Combine Attribute Arrays filter.
 */
class COMPLEXCORE_EXPORT ReadASCIIDataFilter : public IFilter
{
public:
  ReadASCIIDataFilter();
  ~ReadASCIIDataFilter() noexcept override;

  ReadASCIIDataFilter(const ReadASCIIDataFilter&) = delete;
  ReadASCIIDataFilter(ReadASCIIDataFilter&&) noexcept = delete;

  ReadASCIIDataFilter& operator=(const ReadASCIIDataFilter&) = delete;
  ReadASCIIDataFilter& operator=(ReadASCIIDataFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_WizardData_Key = "Wizard Data";
  static inline constexpr StringLiteral k_TupleDims_Key = "Tuple Dimensions";
  static inline constexpr StringLiteral k_UseExistingGroup_Key = "Use Existing Group";
  static inline constexpr StringLiteral k_SelectedDataGroup_Key = "Selected Data Group";
  static inline constexpr StringLiteral k_CreatedDataGroup_Key = "Created Data Group";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ReadASCIIDataFilter, "bdb978bc-96bf-5498-972c-b509c38b8d50");
