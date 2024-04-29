#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT ReadTextDataArrayFilter : public IFilter
{
public:
  ReadTextDataArrayFilter() = default;
  ~ReadTextDataArrayFilter() noexcept override = default;

  ReadTextDataArrayFilter(const ReadTextDataArrayFilter&) = delete;
  ReadTextDataArrayFilter(ReadTextDataArrayFilter&&) noexcept = delete;

  ReadTextDataArrayFilter& operator=(const ReadTextDataArrayFilter&) = delete;
  ReadTextDataArrayFilter& operator=(ReadTextDataArrayFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_ScalarType_Key = "scalar_type_index";
  static inline constexpr StringLiteral k_NTuples_Key = "number_tuples";
  static inline constexpr StringLiteral k_NComp_Key = "number_comp";
  static inline constexpr StringLiteral k_NSkipLines_Key = "skip_line_count";
  static inline constexpr StringLiteral k_DelimiterChoice_Key = "delimiter_index";
  static inline constexpr StringLiteral k_DataArrayPath_Key = "output_data_array_path";
  static inline constexpr StringLiteral k_DataFormat_Key = "data_format";
  static inline constexpr StringLiteral k_AdvancedOptions_Key = "set_tuple_dimensions";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief defaultTags
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The PipelineNode object that called this filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadTextDataArrayFilter, "25f7df3e-ca3e-4634-adda-732c0e56efd4");
