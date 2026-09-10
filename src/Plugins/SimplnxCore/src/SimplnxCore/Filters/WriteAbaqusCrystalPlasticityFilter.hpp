#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteAbaqusCrystalPlasticityFilter
 * @brief Writes a voxel mesh and crystal-plasticity material data to Abaqus input files.
 */
class SIMPLNXCORE_EXPORT WriteAbaqusCrystalPlasticityFilter : public IFilter
{
public:
  WriteAbaqusCrystalPlasticityFilter() = default;
  ~WriteAbaqusCrystalPlasticityFilter() noexcept override = default;

  WriteAbaqusCrystalPlasticityFilter(const WriteAbaqusCrystalPlasticityFilter&) = delete;
  WriteAbaqusCrystalPlasticityFilter(WriteAbaqusCrystalPlasticityFilter&&) noexcept = delete;

  WriteAbaqusCrystalPlasticityFilter& operator=(const WriteAbaqusCrystalPlasticityFilter&) = delete;
  WriteAbaqusCrystalPlasticityFilter& operator=(WriteAbaqusCrystalPlasticityFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_OutputPath_Key = "output_path";
  static constexpr StringLiteral k_FilePrefix_Key = "file_prefix";
  static constexpr StringLiteral k_JobName_Key = "job_name";
  static constexpr StringLiteral k_NumDepvar_Key = "num_depvar";
  static constexpr StringLiteral k_NumUserOutVar_Key = "num_user_out_var";
  static constexpr StringLiteral k_MaterialConstants_Key = "material_constants";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";

  /**
   * @brief Converts a legacy CreateAbaqusFile JSON object to filter arguments.
   * @param json Legacy filter JSON object.
   * @return Converted arguments or conversion errors.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter name.
   * @return Internal filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ class name.
   * @return Filter class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the filter UUID.
   * @return Filter UUID.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the user-visible filter name.
   * @return Human-readable filter name.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the filter tags.
   * @return Tags that describe the filter.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter parameters.
   * @return Parameter definitions in display order.
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
   * @brief Validates the output directory and input tuple counts.
   * @param dataStructure Contains the input geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Empty output actions or validation errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Writes the Abaqus input files.
   * @param dataStructure Contains the input geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param pipelineNode Identifies the executing pipeline node.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Success, cancellation, or a file-writing error.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteAbaqusCrystalPlasticityFilter, "20448af0-c18c-4c76-8092-342b6081eac8");
/* LEGACY UUID FOR THIS FILTER d702beff-eb02-5ee1-a76a-79d5b56ec730 */
