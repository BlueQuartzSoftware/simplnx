#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class FindTriangleGeomShapes
 * @brief This filter will ....
 */
class COMPLEX_EXPORT FindTriangleGeomShapes : public IFilter
{
public:
  FindTriangleGeomShapes() = default;
  ~FindTriangleGeomShapes() noexcept override = default;

  FindTriangleGeomShapes(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes(FindTriangleGeomShapes&&) noexcept = delete;

  FindTriangleGeomShapes& operator=(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes& operator=(FindTriangleGeomShapes&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FaceLabelsArrayPath_Key = "FaceLabelsArrayPath";
  static inline constexpr StringLiteral k_FeatureAttributeMatrixName_Key = "FeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "CentroidsArrayPath";
  static inline constexpr StringLiteral k_VolumesArrayPath_Key = "VolumesArrayPath";
  static inline constexpr StringLiteral k_Omega3sArrayName_Key = "Omega3sArrayName";
  static inline constexpr StringLiteral k_AxisLengthsArrayName_Key = "AxisLengthsArrayName";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayName_Key = "AxisEulerAnglesArrayName";
  static inline constexpr StringLiteral k_AspectRatiosArrayName_Key = "AspectRatiosArrayName";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

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
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex::FindTriangleGeomShapes, "42bcb963-047c-58ab-a6a6-034c9c1f840d");
