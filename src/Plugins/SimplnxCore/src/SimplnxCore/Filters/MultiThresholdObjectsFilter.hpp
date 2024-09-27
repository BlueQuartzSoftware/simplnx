#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT MultiThresholdObjectsFilter : public IFilter
{
public:
  MultiThresholdObjectsFilter() = default;
  ~MultiThresholdObjectsFilter() noexcept override = default;

  MultiThresholdObjectsFilter(const MultiThresholdObjectsFilter&) = delete;
  MultiThresholdObjectsFilter(MultiThresholdObjectsFilter&&) noexcept = delete;

  MultiThresholdObjectsFilter& operator=(const MultiThresholdObjectsFilter&) = delete;
  MultiThresholdObjectsFilter& operator=(MultiThresholdObjectsFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_ArrayThresholdsObject_Key = "array_thresholds_object";
  static inline constexpr StringLiteral k_UseCustomTrueValue = "use_custom_true_value";
  static inline constexpr StringLiteral k_UseCustomFalseValue = "use_custom_false_value";
  static inline constexpr StringLiteral k_CustomTrueValue = "custom_true_value";
  static inline constexpr StringLiteral k_CustomFalseValue = "custom_false_value";
  static inline constexpr StringLiteral k_CreatedDataName_Key = "output_data_array_name";
  static inline constexpr StringLiteral k_CreatedMaskType_Key = "created_mask_type";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  enum ErrorCodes : int64
  {
    PathNotFoundError = -178,
    NonScalarArrayFound = -4001,
    UnequalTuples = -4002,
    CustomTrueWithBoolean = -4003,
    CustomFalseWithBoolean = -4004,
    CustomTrueOutOfBounds = -4005,
    CustomFalseOutOfBounds = -4006
  };

  /**
   * @brief
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return Parameters
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
   * @brief
   * @return IFilter::UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return PreflightResult
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MultiThresholdObjectsFilter, "4246245e-1011-4add-8436-0af6bed19228");
