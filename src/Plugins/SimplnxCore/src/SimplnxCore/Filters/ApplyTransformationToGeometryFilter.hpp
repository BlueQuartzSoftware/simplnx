#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ApplyTransformationToGeometryFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ApplyTransformationToGeometryFilter : public IFilter
{
public:
  ApplyTransformationToGeometryFilter() = default;
  ~ApplyTransformationToGeometryFilter() noexcept override = default;

  ApplyTransformationToGeometryFilter(const ApplyTransformationToGeometryFilter&) = delete;
  ApplyTransformationToGeometryFilter(ApplyTransformationToGeometryFilter&&) noexcept = delete;

  ApplyTransformationToGeometryFilter& operator=(const ApplyTransformationToGeometryFilter&) = delete;
  ApplyTransformationToGeometryFilter& operator=(ApplyTransformationToGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry_path";
  static inline constexpr StringLiteral k_TransformationType_Key = "transformation_type";
  static inline constexpr StringLiteral k_InterpolationType_Key = "interpolation_type";
  static inline constexpr StringLiteral k_ManualTransformationMatrix_Key = "manual_transformation_matrix";
  static inline constexpr StringLiteral k_Rotation_Key = "rotation";
  static inline constexpr StringLiteral k_Translation_Key = "translation";
  static inline constexpr StringLiteral k_Scale_Key = "scale";
  static inline constexpr StringLiteral k_ComputedTransformationMatrix_Key = "computed_transformation_matrix_path";
  static inline constexpr StringLiteral k_TranslateGeometryToGlobalOrigin_Key = "translate_geometry_to_global_origin";
  static inline constexpr StringLiteral k_CellAttributeMatrixPath_Key = "cell_attribute_matrix_path";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ApplyTransformationToGeometryFilter, "f5bbc16b-3426-4ae0-b27b-ba7862dc40fe");
/* LEGACY UUID FOR THIS FILTER c681caf4-22f2-5885-bbc9-a0476abc72eb */
