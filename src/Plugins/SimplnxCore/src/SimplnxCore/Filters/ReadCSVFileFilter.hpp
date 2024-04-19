#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

class AbstractDataParser;

namespace nx::core
{
/**
 * @class ReadCSVFileFilter
 * @brief This filter reads CSV data from any text-based file and imports the data into simplnx-style arrays.
 * The user uses the parameter user interface to specify which file to import, how the data is formatted, what to call
 * each array, and what type each array should be.
 *
 * Note:* This filter is intended to read data that is column-oriented, such that each created simplnx array
 * corresponds to a column of data in the CSV file. Therefore, this filter will only import scalar arrays.
 * If multiple columns are in fact different components of the same array, then the columns may be imported as
 * separate arrays and then combined in the correct order using the Combine Attribute Arrays filter.
 */
class SIMPLNXCORE_EXPORT ReadCSVFileFilter : public IFilter
{
public:
  ReadCSVFileFilter();
  ~ReadCSVFileFilter() noexcept override;

  ReadCSVFileFilter(const ReadCSVFileFilter&) = delete;
  ReadCSVFileFilter(ReadCSVFileFilter&&) noexcept = delete;

  ReadCSVFileFilter& operator=(const ReadCSVFileFilter&) = delete;
  ReadCSVFileFilter& operator=(ReadCSVFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ReadCSVData_Key = "read_csv_data_object";
  static inline constexpr StringLiteral k_UseExistingGroup_Key = "use_existing_group";
  static inline constexpr StringLiteral k_SelectedAttributeMatrixPath_Key = "selected_attribute_matrix_path";
  static inline constexpr StringLiteral k_CreatedDataGroup_Key = "created_data_group_path";

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

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadCSVFileFilter, "373be1f8-31cf-49f6-aa5d-e356f4f3f261");
