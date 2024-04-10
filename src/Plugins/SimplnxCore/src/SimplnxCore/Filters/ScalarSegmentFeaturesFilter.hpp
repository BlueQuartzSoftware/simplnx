#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @class ScalarSegmentFeaturesFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT ScalarSegmentFeaturesFilter : public IFilter
{
public:
  ScalarSegmentFeaturesFilter() = default;
  ~ScalarSegmentFeaturesFilter() noexcept override = default;

  ScalarSegmentFeaturesFilter(const ScalarSegmentFeaturesFilter&) = delete;
  ScalarSegmentFeaturesFilter(ScalarSegmentFeaturesFilter&&) noexcept = delete;

  ScalarSegmentFeaturesFilter& operator=(const ScalarSegmentFeaturesFilter&) = delete;
  ScalarSegmentFeaturesFilter& operator=(ScalarSegmentFeaturesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GridGeomPath_Key = "selected_image_geometry_path";
  static inline constexpr StringLiteral k_ScalarToleranceKey = "scalar_tolerance";
  static inline constexpr StringLiteral k_InputArrayPathKey = "input_array_path";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_path";
  static inline constexpr StringLiteral k_FeatureIdsName_Key = "feature_ids_name";
  static inline constexpr StringLiteral k_CellFeatureName_Key = "cell_feature_group_name";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "active_array_name";
  static inline constexpr StringLiteral k_RandomizeFeatures_Key = "randomize_features";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's Uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable filter name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter's parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ScalarSegmentFeaturesFilter, "e067cd97-9bbf-4c92-89a6-3cb4fdb76c93");
