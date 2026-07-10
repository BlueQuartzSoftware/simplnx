#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WritePoleFigureFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT WritePoleFigureFilter : public IFilter
{
public:
  WritePoleFigureFilter() = default;
  ~WritePoleFigureFilter() noexcept override = default;

  WritePoleFigureFilter(const WritePoleFigureFilter&) = delete;
  WritePoleFigureFilter(WritePoleFigureFilter&&) noexcept = delete;

  WritePoleFigureFilter& operator=(const WritePoleFigureFilter&) = delete;
  WritePoleFigureFilter& operator=(WritePoleFigureFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_Title_Key = "title";
  static constexpr StringLiteral k_GenerationAlgorithm_Key = "generation_algorithm_index";
  static constexpr StringLiteral k_LambertSize_Key = "lambert_size";
  static constexpr StringLiteral k_NumColors_Key = "num_colors";
  static constexpr StringLiteral k_DiscreteMarkerRadius_Key = "discrete_marker_radius";
  static constexpr StringLiteral k_ImageLayout_Key = "image_layout_index";
  static constexpr StringLiteral k_OutputPath_Key = "output_path";
  static constexpr StringLiteral k_ImagePrefix_Key = "image_prefix";
  static constexpr StringLiteral k_ImageSize_Key = "image_size";
  static constexpr StringLiteral k_UseMask_Key = "use_mask";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static constexpr StringLiteral k_MaterialNameArrayPath_Key = "material_name_array_path";
  static constexpr StringLiteral k_SaveAsImageGeometry_Key = "save_as_image_geometry";
  static constexpr StringLiteral k_WriteImageToDisk = "write_image_to_disk";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "output_image_geometry_path";
  static constexpr StringLiteral k_SaveIntensityDataArrays = "save_intensity_plots";
  static constexpr StringLiteral k_NormalizeToMRD = "normalize_to_mrd";
  static constexpr StringLiteral k_IntensityGeometryPath = "intensity_geometry_path";
  static constexpr StringLiteral k_IntensityPlot1Name = "intensity_plot_1_name";
  static constexpr StringLiteral k_IntensityPlot2Name = "intensity_plot_2_name";
  static constexpr StringLiteral k_IntensityPlot3Name = "intensity_plot_3_name";
  static constexpr StringLiteral k_HexConvention_Key = "hex_convention_index";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WritePoleFigureFilter, "00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed");
/* LEGACY UUID FOR THIS FILTER a10bb78e-fcff-553d-97d6-830a43c85385 */
