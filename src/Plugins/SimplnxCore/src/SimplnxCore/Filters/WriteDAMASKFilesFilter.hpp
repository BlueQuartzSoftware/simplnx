#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteDAMASKFilesFilter
 * @brief Writes geometry and material configuration files for DAMASK simulations.
 */
class SIMPLNXCORE_EXPORT WriteDAMASKFilesFilter : public IFilter
{
public:
  WriteDAMASKFilesFilter() = default;
  ~WriteDAMASKFilesFilter() noexcept override = default;

  WriteDAMASKFilesFilter(const WriteDAMASKFilesFilter&) = delete;
  WriteDAMASKFilesFilter(WriteDAMASKFilesFilter&&) noexcept = delete;
  WriteDAMASKFilesFilter& operator=(const WriteDAMASKFilesFilter&) = delete;
  WriteDAMASKFilesFilter& operator=(WriteDAMASKFilesFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_DataFormat_Key = "data_format_index";
  static constexpr StringLiteral k_OutputPath_Key = "output_path";
  static constexpr StringLiteral k_GeometryFileName_Key = "geometry_file_name";
  static constexpr StringLiteral k_HomogenizationIndex_Key = "homogenization_index";
  static constexpr StringLiteral k_CompressGeomFile_Key = "compress_geom_file";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";

  /**
   * @brief Converts a legacy DREAM.3D pipeline filter to simplnx arguments.
   * @param json Legacy filter JSON object.
   * @return Converted arguments or conversion errors.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the internal filter name.
   * @return Internal filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ class name.
   * @return C++ class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the filter UUID.
   * @return Filter UUID.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable filter name.
   * @return Human-readable filter name.
   */
  std::string humanName() const override;

  /**
   * @brief Returns tags that describe the filter.
   * @return Filter tags.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter parameters.
   * @return Filter parameters.
   */
  Parameters parameters() const override;

  /**
   * @brief Returns the parameter schema version.
   * @return Parameter schema version.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Creates a new filter instance.
   * @return New filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates the parameters, geometry, and input tuple counts.
   * @param dataStructure Contains the selected geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param messageHandler Receives preflight messages.
   * @param shouldCancel Indicates that preflight must stop.
   * @param executionContext Resolves relative paths.
   * @return Empty output actions or validation errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Writes both DAMASK files.
   * @param dataStructure Contains the selected geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param pipelineNode Identifies the executing pipeline node.
   * @param messageHandler Receives progress and warning messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Success, cancellation, or an export error.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteDAMASKFilesFilter, "e1aeced3-9fdf-482f-b03b-e43a49444402");
/* LEGACY UUID FOR THIS FILTER 7c58e612-d7d6-5ec7-806b-cce0c1c211a3 */
