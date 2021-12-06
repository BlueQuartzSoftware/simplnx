#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class InterpolatePointCloudToRegularGridFilter
 * @brief
 */
class COMPLEXCORE_EXPORT InterpolatePointCloudToRegularGridFilter : public IFilter
{
public:
  InterpolatePointCloudToRegularGridFilter() = default;
  ~InterpolatePointCloudToRegularGridFilter() noexcept override = default;

  InterpolatePointCloudToRegularGridFilter(const InterpolatePointCloudToRegularGridFilter&) = delete;
  InterpolatePointCloudToRegularGridFilter(InterpolatePointCloudToRegularGridFilter&&) noexcept = delete;

  InterpolatePointCloudToRegularGridFilter& operator=(const InterpolatePointCloudToRegularGridFilter&) = delete;
  InterpolatePointCloudToRegularGridFilter& operator=(InterpolatePointCloudToRegularGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumericType_Key = "numeric_type";
  static inline constexpr StringLiteral k_NumComps_Key = "component_count";
  static inline constexpr StringLiteral k_NumTuples_Key = "tuple_count";
  static inline constexpr StringLiteral k_DataPath_Key = "output_data_array";

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
   * @brief Returns the filter's human-readable name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a collection of filter parameters.
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

COMPLEX_DEF_FILTER_TRAITS(complex, InterpolatePointCloudToRegularGridFilter, "38766252-1200-4c8c-a38a-5c4d97da0f2d");
