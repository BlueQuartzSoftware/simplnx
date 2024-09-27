#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteASCIIDataFilter
 * @brief This filter will export data from **DataArray** as "plaintext" to one or more files according to selection parameters
 */
class SIMPLNXCORE_EXPORT WriteASCIIDataFilter : public IFilter
{
public:
  WriteASCIIDataFilter() = default;
  ~WriteASCIIDataFilter() noexcept override = default;

  WriteASCIIDataFilter(const WriteASCIIDataFilter&) = delete;
  WriteASCIIDataFilter(WriteASCIIDataFilter&&) noexcept = delete;

  WriteASCIIDataFilter& operator=(const WriteASCIIDataFilter&) = delete;
  WriteASCIIDataFilter& operator=(WriteASCIIDataFilter&&) noexcept = delete;

  enum class OutputStyle : uint64
  {
    MultipleFiles = 0,
    SingleFile = 1
  };

  enum class Includes : uint64
  {
    Neither = 0,
    Headers = 1,
    ColumnIndex = 2,
    Both = 3,
  };

  // Parameter Keys
  static inline constexpr StringLiteral k_OutputStyle_Key = "output_style_index";
  static inline constexpr StringLiteral k_OutputPath_Key = "output_path";
  static inline constexpr StringLiteral k_OutputDir_Key = "output_dir";
  static inline constexpr StringLiteral k_FileExtension_Key = "file_extension";
  static inline constexpr StringLiteral k_MaxValPerLine_Key = "max_val_per_line";
  static inline constexpr StringLiteral k_Delimiter_Key = "delimiter_index";
  static inline constexpr StringLiteral k_Includes_Key = "header_option_index";
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "input_data_array_paths";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteASCIIDataFilter, "06c8bfe8-2b42-4956-aca3-580bc0620716");
