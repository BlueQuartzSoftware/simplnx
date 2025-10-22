#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT PointSampleEdgeGeometryFilter : public IFilter
{
public:
  PointSampleEdgeGeometryFilter() = default;
  ~PointSampleEdgeGeometryFilter() noexcept override = default;

  PointSampleEdgeGeometryFilter(const PointSampleEdgeGeometryFilter&) = delete;
  PointSampleEdgeGeometryFilter(PointSampleEdgeGeometryFilter&&) noexcept = delete;

  PointSampleEdgeGeometryFilter& operator=(const PointSampleEdgeGeometryFilter&) = delete;
  PointSampleEdgeGeometryFilter& operator=(PointSampleEdgeGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ScanVectorSamplingRes_Key = "scan_vector_sampling_resolution";
  static constexpr StringLiteral k_ScanVectorGeometryPath_Key = "scan_vector_geometry_path";
  static constexpr StringLiteral k_CalculateCumulativeSampleDistance_Key = "calculate_cumulative_sample_distance";
  static constexpr StringLiteral k_CumulativeSampleDistanceArrayName_Key = "cumulative_sample_distance_array_name";
  static constexpr StringLiteral k_EdgeIdsArrayName_Key = "edge_ids_array_name";
  static constexpr StringLiteral k_SampledVertexGeometryPath_Key = "sampled_vertex_geometry_path";
  static constexpr StringLiteral k_SelectedDataArrayPaths_Key = "input_data_array_paths";
  static constexpr StringLiteral k_VertexDataGroupName_Key = "vertex_data_group_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return std::string
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
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, PointSampleEdgeGeometryFilter, "116d56d1-163c-4ab2-9a8c-234fba0b15c0");
