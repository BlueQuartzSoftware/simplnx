#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class DetectEllipsoids
 * @brief This filter will ....
 */
class PROCESSING_EXPORT DetectEllipsoids : public IFilter
{
public:
  DetectEllipsoids() = default;
  ~DetectEllipsoids() noexcept override = default;

  DetectEllipsoids(const DetectEllipsoids&) = delete;
  DetectEllipsoids(DetectEllipsoids&&) noexcept = delete;

  DetectEllipsoids& operator=(const DetectEllipsoids&) = delete;
  DetectEllipsoids& operator=(DetectEllipsoids&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MinFiberAxisLength_Key = "min_fiber_axis_length";
  static inline constexpr StringLiteral k_MaxFiberAxisLength_Key = "max_fiber_axis_length";
  static inline constexpr StringLiteral k_HoughTransformThreshold_Key = "hough_transform_threshold";
  static inline constexpr StringLiteral k_MinAspectRatio_Key = "min_aspect_ratio";
  static inline constexpr StringLiteral k_ImageScaleBarLength_Key = "image_scale_bar_length";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeatureAttributeMatrixPath_Key = "feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_EllipseFeatureAttributeMatrixPath_Key = "ellipse_feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_CenterCoordinatesArrayName_Key = "center_coordinates_array_name";
  static inline constexpr StringLiteral k_MajorAxisLengthArrayName_Key = "major_axis_length_array_name";
  static inline constexpr StringLiteral k_MinorAxisLengthArrayName_Key = "minor_axis_length_array_name";
  static inline constexpr StringLiteral k_RotationalAnglesArrayName_Key = "rotational_angles_array_name";
  static inline constexpr StringLiteral k_DetectedEllipsoidsFeatureIdsArrayPath_Key = "detected_ellipsoids_feature_ids_array_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, DetectEllipsoids, "033ea842-43c9-424c-9480-fa3042d45ecd");
