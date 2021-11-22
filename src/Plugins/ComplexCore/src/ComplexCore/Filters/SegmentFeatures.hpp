#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class SegmentFeatures
 * @brief The base class for various segment feature classes.
 */
class COMPLEXCORE_EXPORT SegmentFeatures : public IFilter
{
public:
  SegmentFeatures() = default;
  ~SegmentFeatures() noexcept override = default;

  SegmentFeatures(const SegmentFeatures&) = delete;
  SegmentFeatures(SegmentFeatures&&) noexcept = delete;

  SegmentFeatures& operator=(const SegmentFeatures&) = delete;
  SegmentFeatures& operator=(SegmentFeatures&&) noexcept = delete;

  static inline constexpr StringLiteral k_GridGeomPath_Key = "grid_geometry_path";

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
   * @brief Returns the filter's human-readable name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the filter's Parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates a new instance of the filter.
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

  /**
   * @brief Returns the seed for the specified values.
   * @param data
   * @param args
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  virtual int64 getSeed(const DataStructure& data, const Arguments& args, int32 gnum, int64 nextSeed) const;

  /**
   * @brief Determines the grouping for the specified values.
   * @param data
   * @param args
   * @param referencePoint
   * @param neighborPoint
   * @param gnum
   * @return bool
   */
  virtual bool determineGrouping(const DataStructure& data, const Arguments& args, int64 referencePoint, int64 neighborPoint, int32 gnum) const;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, SegmentFeatures, "ae132e6d-bdc3-46fe-9e95-494a485b218f");
