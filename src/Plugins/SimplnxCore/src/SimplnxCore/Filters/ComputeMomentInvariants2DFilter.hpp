#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeMomentInvariants2DFilter
 * @brief This filter computes the 2D Omega-1 and Omega 2 values from the Central Moments matrix and optionally will normalize the values to a unit circle and also optionally save the Central Moments
 * matrix as a DataArray to the Cell Feature Attribute Matrix.
 */
class SIMPLNXCORE_EXPORT ComputeMomentInvariants2DFilter : public IFilter
{
public:
  ComputeMomentInvariants2DFilter() = default;
  ~ComputeMomentInvariants2DFilter() noexcept override = default;

  ComputeMomentInvariants2DFilter(const ComputeMomentInvariants2DFilter&) = delete;
  ComputeMomentInvariants2DFilter(ComputeMomentInvariants2DFilter&&) noexcept = delete;

  ComputeMomentInvariants2DFilter& operator=(const ComputeMomentInvariants2DFilter&) = delete;
  ComputeMomentInvariants2DFilter& operator=(ComputeMomentInvariants2DFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "image_geometry_path";
  static inline constexpr StringLiteral k_FeatureAttributeMatrixPath_Key = "feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static inline constexpr StringLiteral k_FeatureRectArrayPath_Key = "feature_rect_array_path";
  static inline constexpr StringLiteral k_NormalizeMomentInvariants_Key = "normalize_moment_invariants";
  static inline constexpr StringLiteral k_Omega1ArrayPath_Key = "omega1_array_path";
  static inline constexpr StringLiteral k_Omega2ArrayPath_Key = "omega2_array_path";
  static inline constexpr StringLiteral k_SaveCentralMoments_Key = "save_central_moments";
  static inline constexpr StringLiteral k_CentralMomentsArrayPath_Key = "central_moments_array_path";

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeMomentInvariants2DFilter, "650aa454-130c-406e-a9df-80b99f89e03c");
/* LEGACY UUID FOR THIS FILTER 27a132b2-a592-519a-8cb7-38599a7f28ec */
