#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class CreatePythonSkeletonFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT CreatePythonSkeletonFilter : public IFilter
{
public:
  CreatePythonSkeletonFilter() = default;
  ~CreatePythonSkeletonFilter() noexcept override = default;

  CreatePythonSkeletonFilter(const CreatePythonSkeletonFilter&) = delete;
  CreatePythonSkeletonFilter(CreatePythonSkeletonFilter&&) noexcept = delete;

  CreatePythonSkeletonFilter& operator=(const CreatePythonSkeletonFilter&) = delete;
  CreatePythonSkeletonFilter& operator=(CreatePythonSkeletonFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseExistingPlugin_Key = "use_existing_plugin";
  static inline constexpr StringLiteral k_PluginName_Key = "plugin_name";
  static inline constexpr StringLiteral k_PluginHumanName_Key = "plugin_human_name";
  static inline constexpr StringLiteral k_PluginInputDirectory_Key = "plugin_input_directory";
  static inline constexpr StringLiteral k_PluginOutputDirectory_Key = "plugin_output_directory";
  static inline constexpr StringLiteral k_PluginFilterNames = "filter_names";

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
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CreatePythonSkeletonFilter, "1a35f50d-a9f5-9ea2-af70-5b9cf894e45f");
/* LEGACY UUID FOR THIS FILTER ef28de7e-5bdd-57c2-9318-60ba0dfaf7bc */
