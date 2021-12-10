#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MapPointCloudToRegularGridFilter
 * @brief 
 */
class COMPLEXCORE_EXPORT MapPointCloudToRegularGridFilter : public IFilter
{
public:
  MapPointCloudToRegularGridFilter() = default;
  ~MapPointCloudToRegularGridFilter() noexcept override = default;

  MapPointCloudToRegularGridFilter(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  MapPointCloudToRegularGridFilter& operator=(const MapPointCloudToRegularGridFilter&) = delete;
  MapPointCloudToRegularGridFilter& operator=(MapPointCloudToRegularGridFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumericType_Key = "numeric_type";
  static inline constexpr StringLiteral k_NumComps_Key = "component_count";
  static inline constexpr StringLiteral k_NumTuples_Key = "tuple_count";
  static inline constexpr StringLiteral k_DataPath_Key = "output_data_array";

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

COMPLEX_DEF_FILTER_TRAITS(complex, MapPointCloudToRegularGridFilter, "f0134a19-3aad-4c56-ab4a-a0ccb54a7fca");
