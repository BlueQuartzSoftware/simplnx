#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT MinNeighbors : public IFilter
{
public:
  MinNeighbors() = default;
  ~MinNeighbors() noexcept override = default;

  MinNeighbors(const MinNeighbors&) = delete;
  MinNeighbors(MinNeighbors&&) noexcept = delete;

  MinNeighbors& operator=(const MinNeighbors&) = delete;
  MinNeighbors& operator=(MinNeighbors&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImageGeom_Key = "image_geom";
  static inline constexpr StringLiteral k_ApplyToSinglePhase_Key = "apply_to_single_phase";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";
  static inline constexpr StringLiteral k_FeatureIds_Key = "feature_ids";
  static inline constexpr StringLiteral k_FeaturePhases_Key = "feature_phases";
  static inline constexpr StringLiteral k_NumNeighbors_Key = "num_neighbors";
  static inline constexpr StringLiteral k_MinNumNeighbors_Key = "min_num_neighbors";
  static inline constexpr StringLiteral k_VoxelArrays_Key = "voxel_arrays";

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MinNeighbors, "dab5de3c-5f81-5bb5-8490-73521e1183ea");
