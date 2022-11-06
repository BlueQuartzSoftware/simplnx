#pragma once

#include "EMMPM/EMMPM_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MultiEmmpmFilter
 * @brief This filter will ....
 */
class EMMPM_EXPORT MultiEmmpmFilter : public IFilter
{
public:
  MultiEmmpmFilter() = default;
  ~MultiEmmpmFilter() noexcept override = default;

  MultiEmmpmFilter(const MultiEmmpmFilter&) = delete;
  MultiEmmpmFilter(MultiEmmpmFilter&&) noexcept = delete;

  MultiEmmpmFilter& operator=(const MultiEmmpmFilter&) = delete;
  MultiEmmpmFilter& operator=(MultiEmmpmFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumClasses_Key = "num_classes";
  static inline constexpr StringLiteral k_UseOneBasedValues_Key = "use_one_based_values";
  static inline constexpr StringLiteral k_UseGradientPenalty_Key = "use_gradient_penalty";
  static inline constexpr StringLiteral k_GradientBetaE_Key = "gradient_beta_e";
  static inline constexpr StringLiteral k_UseCurvaturePenalty_Key = "use_curvature_penalty";
  static inline constexpr StringLiteral k_CurvatureBetaC_Key = "curvature_beta_c";
  static inline constexpr StringLiteral k_CurvatureRMax_Key = "curvature_rmax";
  static inline constexpr StringLiteral k_CurvatureEMLoopDelay_Key = "curvature_em_loop_delay";
  static inline constexpr StringLiteral k_InputDataArrayVector_Key = "input_data_array_vector";
  static inline constexpr StringLiteral k_OutputAttributeMatrixName_Key = "output_attribute_matrix_name";
  static inline constexpr StringLiteral k_UsePreviousMuSigma_Key = "use_previous_mu_sigma";
  static inline constexpr StringLiteral k_OutputArrayPrefix_Key = "output_array_prefix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, MultiEmmpmFilter, "82be6e42-e174-480c-8e9b-e798bbb72f07");
