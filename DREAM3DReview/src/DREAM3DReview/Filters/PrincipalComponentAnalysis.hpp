#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class PrincipalComponentAnalysis
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT PrincipalComponentAnalysis : public IFilter
{
public:
  PrincipalComponentAnalysis() = default;
  ~PrincipalComponentAnalysis() noexcept override = default;

  PrincipalComponentAnalysis(const PrincipalComponentAnalysis&) = delete;
  PrincipalComponentAnalysis(PrincipalComponentAnalysis&&) noexcept = delete;

  PrincipalComponentAnalysis& operator=(const PrincipalComponentAnalysis&) = delete;
  PrincipalComponentAnalysis& operator=(PrincipalComponentAnalysis&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "selected_data_array_paths";
  static inline constexpr StringLiteral k_MatrixApproach_Key = "matrix_approach";
  static inline constexpr StringLiteral k_ProjectDataSpace_Key = "project_data_space";
  static inline constexpr StringLiteral k_NumberOfDimensionsForProjection_Key = "number_of_dimensions_for_projection";
  static inline constexpr StringLiteral k_ProjectedDataSpaceArrayPath_Key = "projected_data_space_array_path";
  static inline constexpr StringLiteral k_PCAttributeMatrixName_Key = "p_cattribute_matrix_name";
  static inline constexpr StringLiteral k_PCEigenvaluesName_Key = "p_ceigenvalues_name";
  static inline constexpr StringLiteral k_PCEigenvectorsName_Key = "p_ceigenvectors_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, PrincipalComponentAnalysis, "48d7200b-26ec-4ed9-8d1c-c8111118508a");
