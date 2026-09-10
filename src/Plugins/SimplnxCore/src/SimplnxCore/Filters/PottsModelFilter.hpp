#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class PottsModelFilter
 * @brief Applies Monte Carlo Potts-model coarsening to feature IDs in an image geometry.
 */
class SIMPLNXCORE_EXPORT PottsModelFilter : public IFilter
{
public:
  /**
   * @brief Constructs the filter.
   */
  PottsModelFilter() = default;

  /**
   * @brief Destroys the filter.
   */
  ~PottsModelFilter() noexcept override = default;

  PottsModelFilter(const PottsModelFilter&) = delete;
  PottsModelFilter(PottsModelFilter&&) noexcept = delete;

  PottsModelFilter& operator=(const PottsModelFilter&) = delete;
  PottsModelFilter& operator=(PottsModelFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_Iterations_Key = "iterations";
  static constexpr StringLiteral k_Temperature_Key = "temperature";
  static constexpr StringLiteral k_PeriodicBoundaries_Key = "periodic_boundaries";
  static constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static constexpr StringLiteral k_UseMask_Key = "use_mask";
  static constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";

  /**
   * @brief Converts legacy SIMPL JSON values to simplnx arguments.
   * @param json Legacy SIMPL filter JSON object.
   * @return Converted arguments or conversion errors.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter name.
   * @return Registered filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ class name.
   * @return C++ class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the filter UUID.
   * @return Filter UUID.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable filter name.
   * @return Human-readable filter name.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default filter tags.
   * @return Default filter tags.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter parameters.
   * @return Filter parameters.
   */
  Parameters parameters() const override;

  /**
   * @brief Returns the parameter schema version.
   * @return Parameter schema version.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Creates a new filter instance.
   * @return New filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates input values and declares data-structure changes.
   * @param dataStructure Data structure that contains the selected inputs.
   * @param filterArgs Filter parameter values.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that the filter must stop.
   * @param executionContext Resolves relative paths.
   * @return Preflight actions or validation errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Executes the Potts-model algorithm.
   * @param dataStructure Data structure that contains the selected inputs and outputs.
   * @param filterArgs Filter parameter values.
   * @param pipelineNode Pipeline node that owns this execution.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that the filter must stop.
   * @param executionContext Resolves relative paths.
   * @return Execution result.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, PottsModelFilter, "7134a934-af08-4dd1-a57c-0c76c5af476c");
/* LEGACY UUID FOR THIS FILTER e15ec84b-1e02-53a6-a830-59e0813775a1 */
