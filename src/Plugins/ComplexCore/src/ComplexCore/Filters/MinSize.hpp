#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MinSize
 * @brief
 */
class COMPLEXCORE_EXPORT MinSize : public IFilter
{
public:
  MinSize() = default;
  ~MinSize() noexcept override = default;

  MinSize(const MinSize&) = delete;
  MinSize(MinSize&&) noexcept = delete;

  MinSize& operator=(const MinSize&) = delete;
  MinSize& operator=(MinSize&&) noexcept = delete;

  static inline constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_Phiases_path";
  static inline constexpr StringLiteral k_NumCellsPath_Key = "num_cells_path";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "image_geom_path";
  static inline constexpr StringLiteral k_ApplySinglePhase_Key = "apply_single_phase";
  static inline constexpr StringLiteral k_MinAllowedFeaturesSize_Key = "min_allowed_features_size";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, MinSize, "ed8b5905-d000-4ce5-89ee-bd3c4e4094da");
