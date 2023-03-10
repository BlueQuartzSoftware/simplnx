#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class CalcDewarpParameters
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT CalcDewarpParameters : public IFilter
{
public:
  CalcDewarpParameters() = default;
  ~CalcDewarpParameters() noexcept override = default;

  CalcDewarpParameters(const CalcDewarpParameters&) = delete;
  CalcDewarpParameters(CalcDewarpParameters&&) noexcept = delete;

  CalcDewarpParameters& operator=(const CalcDewarpParameters&) = delete;
  CalcDewarpParameters& operator=(CalcDewarpParameters&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MontageName_Key = "montage_name";
  static inline constexpr StringLiteral k_MaxIterations_Key = "max_iterations";
  static inline constexpr StringLiteral k_Delta_Key = "delta";
  static inline constexpr StringLiteral k_FractionalTolerance_Key = "fractional_tolerance";
  static inline constexpr StringLiteral k_SpecifyInitialSimplex_Key = "specify_initial_simplex";
  static inline constexpr StringLiteral k_XFactors_Key = "x_factors";
  static inline constexpr StringLiteral k_YFactors_Key = "y_factors";
  static inline constexpr StringLiteral k_AttributeMatrixName_Key = "attribute_matrix_name";
  static inline constexpr StringLiteral k_IPFColorsArrayName_Key = "i_pf_colors_array_name";
  static inline constexpr StringLiteral k_TransformDCName_Key = "transform_dc_name";
  static inline constexpr StringLiteral k_TransformMatrixName_Key = "transform_matrix_name";
  static inline constexpr StringLiteral k_TransformArrayName_Key = "transform_array_name";

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CalcDewarpParameters, "bd12a81d-730c-41d6-8dad-05d782d2e2d7");
