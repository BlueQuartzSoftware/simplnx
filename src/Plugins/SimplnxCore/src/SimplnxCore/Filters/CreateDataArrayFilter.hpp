#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT CreateDataArrayFilter : public IFilter
{
public:
  CreateDataArrayFilter() = default;
  ~CreateDataArrayFilter() noexcept override = default;

  CreateDataArrayFilter(const CreateDataArrayFilter&) = delete;
  CreateDataArrayFilter(CreateDataArrayFilter&&) noexcept = delete;

  CreateDataArrayFilter& operator=(const CreateDataArrayFilter&) = delete;
  CreateDataArrayFilter& operator=(CreateDataArrayFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumericType_Key = "numeric_type_index";
  static inline constexpr StringLiteral k_AdvancedOptions_Key = "set_tuple_dimensions";
  static inline constexpr StringLiteral k_NumComps_Key = "component_count";
  static inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
  static inline constexpr StringLiteral k_DataPath_Key = "output_array_path";
  static inline constexpr StringLiteral k_DataFormat_Key = "data_format";
  static inline constexpr StringLiteral k_InitType_Key = "init_type_index";
  static inline constexpr StringLiteral k_InitValue_Key = "init_value";
  static inline constexpr StringLiteral k_StartingFillValue_Key = "starting_fill_value";
  static inline constexpr StringLiteral k_StepOperation_Key = "step_operation_index";
  static inline constexpr StringLiteral k_StepValue_Key = "step_value";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static inline constexpr StringLiteral k_InitStartRange_Key = "init_start_range";
  static inline constexpr StringLiteral k_InitEndRange_Key = "init_end_range";
  static inline constexpr StringLiteral k_StandardizeSeed_Key = "standardize_seed";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter name as a human-readable string.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters required to execute the filter.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CreateDataArrayFilter, "67041f9b-bdc6-4122-acc6-c9fe9280e90d");
