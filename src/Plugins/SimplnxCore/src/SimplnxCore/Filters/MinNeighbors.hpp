#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT MinNeighbors : public IFilter
{
public:
  MinNeighbors() = default;
  ~MinNeighbors() noexcept override = default;

  MinNeighbors(const MinNeighbors&) = delete;
  MinNeighbors(MinNeighbors&&) noexcept = delete;

  MinNeighbors& operator=(const MinNeighbors&) = delete;
  MinNeighbors& operator=(MinNeighbors&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "selected_image_geometry_path";
  static inline constexpr StringLiteral k_ApplyToSinglePhase_Key = "apply_to_single_phase";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_phases_path";
  static inline constexpr StringLiteral k_NumNeighborsPath_Key = "num_neighbors_path";
  static inline constexpr StringLiteral k_MinNumNeighbors_Key = "min_num_neighbors";
  static inline constexpr StringLiteral k_IgnoredVoxelArrays_Key = "ignored_voxel_arrays";
  static inline constexpr StringLiteral k_CellDataAttributeMatrixPath_Key = "cell_attribute_matrix_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return std::name
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return IFilter::UniquePointer
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, MinNeighbors, "4ab5153f-6014-4e6d-bbd6-194068620389");
