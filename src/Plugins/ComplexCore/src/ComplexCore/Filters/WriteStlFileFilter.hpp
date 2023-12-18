#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class WriteStlFileFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT WriteStlFileFilter : public IFilter
{
public:
  WriteStlFileFilter() = default;
  ~WriteStlFileFilter() noexcept override = default;

  WriteStlFileFilter(const WriteStlFileFilter&) = delete;
  WriteStlFileFilter(WriteStlFileFilter&&) noexcept = delete;

  WriteStlFileFilter& operator=(const WriteStlFileFilter&) = delete;
  WriteStlFileFilter& operator=(WriteStlFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GroupByFeature = "group_by_feature";
  static inline constexpr StringLiteral k_OutputStlDirectory_Key = "output_stl_directory";
  static inline constexpr StringLiteral k_OutputStlPrefix_Key = "output_stl_prefix";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_phases_path";
  static inline constexpr StringLiteral k_TriangleGeomPath_Key = "triangle_geom_path";
  // static inline constexpr StringLiteral k_FaceNormalsPath_Key = "face_normals_path";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, WriteStlFileFilter, "54a293f4-1366-46ca-b284-fe5965545dd2");
/* LEGACY UUID FOR THIS FILTER b9134758-d5e5-59dd-9907-28d23e0e0143 */
