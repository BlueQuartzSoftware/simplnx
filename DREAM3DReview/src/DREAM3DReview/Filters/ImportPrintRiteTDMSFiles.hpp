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
  static inline constexpr StringLiteral k_LayerThickness_Key = "LayerThickness";
  static inline constexpr StringLiteral k_LaserOnArrayOption_Key = "LaserOnArrayOption";
  static inline constexpr StringLiteral k_LaserOnThreshold_Key = "LaserOnThreshold";
  static inline constexpr StringLiteral k_DowncastRawData_Key = "DowncastRawData";
  static inline constexpr StringLiteral k_ScaleLaserPower_Key = "ScaleLaserPower";
  static inline constexpr StringLiteral k_PowerScalingCoefficients_Key = "PowerScalingCoefficients";
  static inline constexpr StringLiteral k_ScalePyrometerTemperature_Key = "ScalePyrometerTemperature";
  static inline constexpr StringLiteral k_TemperatureScalingCoefficients_Key = "TemperatureScalingCoefficients";
  static inline constexpr StringLiteral k_SpatialTransformOption_Key = "SpatialTransformOption";
  static inline constexpr StringLiteral k_LayerForScaling_Key = "LayerForScaling";
  static inline constexpr StringLiteral k_SearchRadius_Key = "SearchRadius";
  static inline constexpr StringLiteral k_SplitRegions1_Key = "SplitRegions1";
  static inline constexpr StringLiteral k_SplitRegions2_Key = "SplitRegions2";
  static inline constexpr StringLiteral k_STLFilePath1_Key = "STLFilePath1";
  static inline constexpr StringLiteral k_STLFilePath2_Key = "STLFilePath2";
  static inline constexpr StringLiteral k_InputSpatialTransformFilePath_Key = "InputSpatialTransformFilePath";
  static inline constexpr StringLiteral k_InputFilesList_Key = "InputFilesList";
  static inline constexpr StringLiteral k_OutputDirectory_Key = "OutputDirectory";
  static inline constexpr StringLiteral k_OutputFilePrefix_Key = "OutputFilePrefix";

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
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ImportPrintRiteTDMSFiles, "fe5d3f37-0dbc-5493-bc96-b78d18116b89");
