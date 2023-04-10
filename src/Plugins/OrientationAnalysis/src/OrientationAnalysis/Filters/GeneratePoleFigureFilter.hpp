#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class GeneratePoleFigureFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT GeneratePoleFigureFilter : public IFilter
{
public:
  GeneratePoleFigureFilter() = default;
  ~GeneratePoleFigureFilter() noexcept override = default;

  GeneratePoleFigureFilter(const GeneratePoleFigureFilter&) = delete;
  GeneratePoleFigureFilter(GeneratePoleFigureFilter&&) noexcept = delete;

  GeneratePoleFigureFilter& operator=(const GeneratePoleFigureFilter&) = delete;
  GeneratePoleFigureFilter& operator=(GeneratePoleFigureFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GenerationAlgorithm_Key = "generation_algorithm";
  static inline constexpr StringLiteral k_LambertSize_Key = "lambert_size";
  static inline constexpr StringLiteral k_NumColors_Key = "num_colors";
  static inline constexpr StringLiteral k_ImageSize_Key = "image_size";

  static inline constexpr StringLiteral k_UseGoodVoxels_Key = "use_good_voxels";
  static inline constexpr StringLiteral k_GoodVoxelsPath_Key = "good_voxels_array_path";

  static inline constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";

  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_MaterialNameArrayPath_Key = "material_name_array_path";

  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_DataArrayPrefix_Key = "pole_figure_output_array_prefix";
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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, GeneratePoleFigureFilter, "00cbb97e-a5c2-fafb-9a35-17a0f9ce26ed");
/* LEGACY UUID FOR THIS FILTER a10bb78e-fcff-553d-97d6-830a43c85385 */
