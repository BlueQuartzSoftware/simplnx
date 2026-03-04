#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/FilterTraits.hpp"

namespace nx::core
{
/**
 * @class ReadGrainMapper3DFilter
 * @brief This filter determines the average C-axis location of each Feature
 */
class ORIENTATIONANALYSIS_EXPORT ReadGrainMapper3DFilter : public AbstractFilter
{
public:
  ReadGrainMapper3DFilter() = default;
  ~ReadGrainMapper3DFilter() noexcept override = default;

  ReadGrainMapper3DFilter(const ReadGrainMapper3DFilter&) = delete;
  ReadGrainMapper3DFilter(ReadGrainMapper3DFilter&&) noexcept = delete;

  ReadGrainMapper3DFilter& operator=(const ReadGrainMapper3DFilter&) = delete;
  ReadGrainMapper3DFilter& operator=(ReadGrainMapper3DFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_ReadLabDCT_Key = "read_lab_dct_data";
  static constexpr StringLiteral k_CreatedDCTImageGeometryPath_Key = "output_dct_image_geometry_path";
  static constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "cell_ensemble_attribute_matrix_name";
  static constexpr StringLiteral k_ConvertPhaseToInt32_Key = "convert_phase_to_int32";
  static constexpr StringLiteral k_ConvertOrientationData_Key = "convert_orientation_data";
  static constexpr StringLiteral k_ReadAbsorptionCT_Key = "read_absorption_ct_data";
  static constexpr StringLiteral k_CreatedAbsorptionGeometryPath_Key = "output_absorption_image_geometry_path";
  static constexpr StringLiteral k_CellAbsorptionAttributeMatrixName_Key = "cell_absorption_attribute_matrix_name";
  static constexpr StringLiteral k_ConvertIPFColorData_Key = "convert_ipfcolor_data";

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
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
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
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
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
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadGrainMapper3DFilter, "ed46afcf-de32-4f37-98bc-8f0fd4b3c122");
/* LEGACY UUID FOR THIS FILTER */
