#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ApproximatePointCloudHull
 * @brief
 */
class COMPLEXCORE_EXPORT ApproximatePointCloudHull : public IFilter
{
public:
  ApproximatePointCloudHull() = default;
  ~ApproximatePointCloudHull() noexcept override = default;

  ApproximatePointCloudHull(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull(ApproximatePointCloudHull&&) noexcept = delete;

  ApproximatePointCloudHull& operator=(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull& operator=(ApproximatePointCloudHull&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GridResolution_Key = "grid_resolution";
  static inline constexpr StringLiteral k_MinEmptyNeighbors_Key = "min_empty_neighbors";
  static inline constexpr StringLiteral k_VertexGeomPath_Key = "vertex_geometry";
  static inline constexpr StringLiteral k_HullVertexGeomPath_Key = "hull_vertex_geometry";

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
   * @brief Returns the filter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the user-friendly filter name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a copy of the filter's parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return UniquePointer
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

COMPLEX_DEF_FILTER_TRAITS(complex, ApproximatePointCloudHull, "fab669ad-66c6-5a39-bdb7-fc47b94311ed");
