#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT ReadTextDataArrayFilter : public IFilter
{
public:
  ReadTextDataArrayFilter() = default;
  ~ReadTextDataArrayFilter() noexcept override = default;

  ReadTextDataArrayFilter(const ReadTextDataArrayFilter&) = delete;
  ReadTextDataArrayFilter(ReadTextDataArrayFilter&&) noexcept = delete;

  ReadTextDataArrayFilter& operator=(const ReadTextDataArrayFilter&) = delete;
  ReadTextDataArrayFilter& operator=(ReadTextDataArrayFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_InputFileKey = "input_file";
  static inline constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
  static inline constexpr StringLiteral k_NTuplesKey = "n_tuples";
  static inline constexpr StringLiteral k_NCompKey = "n_comp";
  static inline constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
  static inline constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
  static inline constexpr StringLiteral k_DataArrayKey = "output_data_array";
  static inline constexpr StringLiteral k_DataFormat_Key = "data_format";
  static inline constexpr StringLiteral k_AdvancedOptions_Key = "advanced_options";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ReadTextDataArrayFilter, "25f7df3e-ca3e-4634-adda-732c0e56efd4");
