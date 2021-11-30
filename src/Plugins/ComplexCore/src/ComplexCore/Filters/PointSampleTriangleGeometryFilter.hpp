#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class PointSampleTriangleGeometryFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT PointSampleTriangleGeometryFilter : public IFilter
{
public:
  PointSampleTriangleGeometryFilter() = default;
  ~PointSampleTriangleGeometryFilter() noexcept override = default;

  PointSampleTriangleGeometryFilter(const PointSampleTriangleGeometryFilter&) = delete;
  PointSampleTriangleGeometryFilter(PointSampleTriangleGeometryFilter&&) noexcept = delete;

  PointSampleTriangleGeometryFilter& operator=(const PointSampleTriangleGeometryFilter&) = delete;
  PointSampleTriangleGeometryFilter& operator=(PointSampleTriangleGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_VertexParentGroup_Key = "VertexGeometryParentGroup";
  static inline constexpr StringLiteral k_SamplesNumberType_Key = "SamplesNumberType";
  static inline constexpr StringLiteral k_NumberOfSamples_Key = "NumberOfSamples";
  static inline constexpr StringLiteral k_UseMask_Key = "UseMask";
  static inline constexpr StringLiteral k_TriangleGeometry_Key = "TriangleGeometry";
  static inline constexpr StringLiteral k_ParentGeometry_Key = "ParentGeometry";
  static inline constexpr StringLiteral k_TriangleAreasArrayPath_Key = "TriangleAreasArrayPath";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "MaskArrayPath";
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "SelectedDataArrayPaths";
  static inline constexpr StringLiteral k_VertexGeometryName_Key = "VertexGeometry";
  static inline constexpr StringLiteral k_VertexData_DataPath_Key = "VertexAttributeMatrixName";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, PointSampleTriangleGeometryFilter, "ee34ef95-aa04-5ad3-8232-5783a880d279");
