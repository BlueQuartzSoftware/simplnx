#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FeatureDataCSVWriterFilter
 * @brief This filter will export feature data as ASCII to a single file using choice delimiter
 */
class CORE_EXPORT FeatureDataCSVWriterFilter : public IFilter
{
public:
  FeatureDataCSVWriterFilter() = default;
  ~FeatureDataCSVWriterFilter() noexcept override = default;

  FeatureDataCSVWriterFilter(const FeatureDataCSVWriterFilter&) = delete;
  FeatureDataCSVWriterFilter(FeatureDataCSVWriterFilter&&) noexcept = delete;

  FeatureDataCSVWriterFilter& operator=(const FeatureDataCSVWriterFilter&) = delete;
  FeatureDataCSVWriterFilter& operator=(FeatureDataCSVWriterFilter&&) noexcept = delete;

  enum class Delimiter : uint64
  {
    Space = 0,
    Semicolon = 1,
    Comma = 2,
    Colon = 3,
    Tab = 4
  };

  // Parameter Keys
  static inline constexpr StringLiteral k_FeatureDataFile_Key = "feature_data_file";
  static inline constexpr StringLiteral k_WriteNeighborListData_Key = "write_neighborlist_data";
  static inline constexpr StringLiteral k_WriteNumFeaturesLine_Key = "write_num_features_line";
  static inline constexpr StringLiteral k_DelimiterChoiceInt_Key = "delimiter_choice_int";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "cell_feature_attribute_matrix_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FeatureDataCSVWriterFilter, "d734293f-3017-4699-b458-c05aca078b96");
