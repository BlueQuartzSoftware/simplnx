#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RemoveMinimumSizeFeaturesFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT RemoveMinimumSizeFeaturesFilter : public IFilter
{
public:
  RemoveMinimumSizeFeaturesFilter() = default;
  ~RemoveMinimumSizeFeaturesFilter() noexcept override = default;

  RemoveMinimumSizeFeaturesFilter(const RemoveMinimumSizeFeaturesFilter&) = delete;
  RemoveMinimumSizeFeaturesFilter(RemoveMinimumSizeFeaturesFilter&&) noexcept = delete;

  RemoveMinimumSizeFeaturesFilter& operator=(const RemoveMinimumSizeFeaturesFilter&) = delete;
  RemoveMinimumSizeFeaturesFilter& operator=(RemoveMinimumSizeFeaturesFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_phases_path";
  static inline constexpr StringLiteral k_NumCellsPath_Key = "num_cells_path";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_ApplySinglePhase_Key = "apply_single_phase";
  static inline constexpr StringLiteral k_MinAllowedFeaturesSize_Key = "min_allowed_features_size";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RemoveMinimumSizeFeaturesFilter, "074472d3-ba8d-4a1a-99f2-2d56a0d082a0");
