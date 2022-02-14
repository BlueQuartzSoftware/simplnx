#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT IterativeClosestPointFilter : public IFilter
{
public:
  IterativeClosestPointFilter() = default;
  ~IterativeClosestPointFilter() noexcept override = default;

  IterativeClosestPointFilter(const IterativeClosestPointFilter&) = delete;
  IterativeClosestPointFilter(IterativeClosestPointFilter&&) noexcept = delete;

  IterativeClosestPointFilter& operator=(const IterativeClosestPointFilter&) = delete;
  IterativeClosestPointFilter& operator=(IterativeClosestPointFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MovingVertexPath_Key = "moving_vertex";
  static inline constexpr StringLiteral k_TargetVertexPath_Key = "target_vertex";
  static inline constexpr StringLiteral k_NumIterations_Key = "num_iterations";
  static inline constexpr StringLiteral k_ApplyTransformation_Key = "apply_transformation";
  static inline constexpr StringLiteral k_TransformArrayPath_Key = "transform_array";

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
   * @return Result<OutputActions>
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

COMPLEX_DEF_FILTER_TRAITS(complex, IterativeClosestPointFilter, "6c8fb24b-5b12-551c-ba6d-ae2fa7724764");
