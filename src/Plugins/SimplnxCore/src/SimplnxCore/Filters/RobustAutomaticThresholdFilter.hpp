#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT RobustAutomaticThresholdFilter : public IFilter
{
public:
  RobustAutomaticThresholdFilter() = default;
  ~RobustAutomaticThresholdFilter() noexcept override = default;

  RobustAutomaticThresholdFilter(const RobustAutomaticThresholdFilter&) = delete;
  RobustAutomaticThresholdFilter(RobustAutomaticThresholdFilter&&) noexcept = delete;

  RobustAutomaticThresholdFilter& operator=(const RobustAutomaticThresholdFilter&) = delete;
  RobustAutomaticThresholdFilter& operator=(RobustAutomaticThresholdFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_InputArrayPath_Key = "input_array_path";
  static inline constexpr StringLiteral k_GradientMagnitudePath_Key = "gradient_array_path";
  static inline constexpr StringLiteral k_ArrayCreationName_Key = "created_mask_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's Uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the name to display to filter's users.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter's Parameters.
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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RobustAutomaticThresholdFilter, "ade392e6-f0da-4cf3-bf11-dfe69e200b49");
