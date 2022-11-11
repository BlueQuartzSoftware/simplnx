#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class WriteASCIIDataFilter
 * @brief This filter will export data from **DataArray** as "plaintext" to one or more files according to selection parameters
 */
class CORE_EXPORT WriteASCIIDataFilter : public IFilter
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
  static inline constexpr StringLiteral k_OutputStyle_Key = "output_style";
  static inline constexpr StringLiteral k_OutputPath_Key = "Output_path";
  static inline constexpr StringLiteral k_FileName_Key = "file_name";
  static inline constexpr StringLiteral k_FileExtension_Key = "file_extension";
  static inline constexpr StringLiteral k_MaxValPerLine_Key = "max_val_per_line";
  static inline constexpr StringLiteral k_Delimiter_Key = "delimiter";
  static inline constexpr StringLiteral k_Includes_Key = "includes";
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "selected_data_array_paths";

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

COMPLEX_DEF_FILTER_TRAITS(complex, WriteASCIIDataFilter, "06c8bfe8-2b42-4956-aca3-580bc0620716");
