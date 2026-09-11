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
  static constexpr StringLiteral k_UseReducedIntegration_Key = "use_reduced_integration";
  static constexpr StringLiteral k_HourglassStiffness_Key = "hourglass_stiffness";
  static constexpr StringLiteral k_NumDepvar_Key = "num_depvar";
  static constexpr StringLiteral k_NumUserOutVar_Key = "num_user_out_var";
  static constexpr StringLiteral k_MaterialConstants_Key = "material_constants";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
   * @brief Returns the human-readable name of the filter.
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
   * @brief Returns parameters version integer.
   * The initial version is 1.
   * The filter increments the version each time the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that determines the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that determines the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteAbaqusCrystalPlasticityFilter, "20448af0-c18c-4c76-8092-342b6081eac8");
/* LEGACY UUID FOR THIS FILTER d702beff-eb02-5ee1-a76a-79d5b56ec730 */
