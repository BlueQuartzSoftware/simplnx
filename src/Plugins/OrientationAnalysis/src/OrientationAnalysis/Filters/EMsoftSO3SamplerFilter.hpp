#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class EMsoftSO3SamplerFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT EMsoftSO3SamplerFilter : public IFilter
{
public:
  EMsoftSO3SamplerFilter() = default;
  ~EMsoftSO3SamplerFilter() noexcept override = default;

  EMsoftSO3SamplerFilter(const EMsoftSO3SamplerFilter&) = delete;
  EMsoftSO3SamplerFilter(EMsoftSO3SamplerFilter&&) noexcept = delete;

  EMsoftSO3SamplerFilter& operator=(const EMsoftSO3SamplerFilter&) = delete;
  EMsoftSO3SamplerFilter& operator=(EMsoftSO3SamplerFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_SampleModeSelector_Key = "sample_mode_index";
  static constexpr StringLiteral k_OffsetGrid_Key = "offset_grid";

  static constexpr StringLiteral k_Mode1Misorientation_Key = "mode_1_misorientation";
  static constexpr StringLiteral k_Mode1EulerAngle_Key = "mode_1_euler_angle";

  static constexpr StringLiteral k_Mode2Misorientation_Key = "mode_2_misorientation";
  static constexpr StringLiteral k_Mode2EulerAngle_Key = "mode_2_euler_angle";

  static constexpr StringLiteral k_NumberSamples_Key = "number_of_samples";
  static constexpr StringLiteral k_EulerAnglesArrayPath_Key = "output_euler_angles_path";

  static constexpr StringLiteral k_CrystalStructure_Index = "crystal_structure_index";

  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_EnsembleAttributeMatrixPath_Key = "cell_ensemble_attribute_matrix_path";

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
   * @brief Returns the human-readable name of the filter.
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
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, EMsoftSO3SamplerFilter, "74478e86-ce29-40b8-8c17-d20009195f91");
/* LEGACY UUID FOR THIS FILTER b78d8825-d3ac-5351-be20-172f07fd2aec */
