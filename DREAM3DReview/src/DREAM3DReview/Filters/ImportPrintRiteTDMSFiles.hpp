#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ImportPrintRiteTDMSFiles
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT ImportPrintRiteTDMSFiles : public IFilter
{
public:
  ImportPrintRiteTDMSFiles() = default;
  ~ImportPrintRiteTDMSFiles() noexcept override = default;

  ImportPrintRiteTDMSFiles(const ImportPrintRiteTDMSFiles&) = delete;
  ImportPrintRiteTDMSFiles(ImportPrintRiteTDMSFiles&&) noexcept = delete;

  ImportPrintRiteTDMSFiles& operator=(const ImportPrintRiteTDMSFiles&) = delete;
  ImportPrintRiteTDMSFiles& operator=(ImportPrintRiteTDMSFiles&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_LayerThickness_Key = "layer_thickness";
  static inline constexpr StringLiteral k_LaserOnArrayOption_Key = "laser_on_array_option";
  static inline constexpr StringLiteral k_LaserOnThreshold_Key = "laser_on_threshold";
  static inline constexpr StringLiteral k_DowncastRawData_Key = "downcast_raw_data";
  static inline constexpr StringLiteral k_ScaleLaserPower_Key = "scale_laser_power";
  static inline constexpr StringLiteral k_PowerScalingCoefficients_Key = "power_scaling_coefficients";
  static inline constexpr StringLiteral k_ScalePyrometerTemperature_Key = "scale_pyrometer_temperature";
  static inline constexpr StringLiteral k_TemperatureScalingCoefficients_Key = "temperature_scaling_coefficients";
  static inline constexpr StringLiteral k_SpatialTransformOption_Key = "spatial_transform_option";
  static inline constexpr StringLiteral k_LayerForScaling_Key = "layer_for_scaling";
  static inline constexpr StringLiteral k_SearchRadius_Key = "search_radius";
  static inline constexpr StringLiteral k_SplitRegions1_Key = "split_regions1";
  static inline constexpr StringLiteral k_SplitRegions2_Key = "split_regions2";
  static inline constexpr StringLiteral k_STLFilePath1_Key = "s_tl_file_path1";
  static inline constexpr StringLiteral k_STLFilePath2_Key = "s_tl_file_path2";
  static inline constexpr StringLiteral k_InputSpatialTransformFilePath_Key = "input_spatial_transform_file_path";
  static inline constexpr StringLiteral k_InputFilesList_Key = "input_files_list";
  static inline constexpr StringLiteral k_OutputDirectory_Key = "output_directory";
  static inline constexpr StringLiteral k_OutputFilePrefix_Key = "output_file_prefix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ImportPrintRiteTDMSFiles, "60a8c8f1-c877-48cb-891f-df4e4a3a3535");
