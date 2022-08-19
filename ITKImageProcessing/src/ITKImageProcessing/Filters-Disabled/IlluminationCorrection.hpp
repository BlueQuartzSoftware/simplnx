#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class IlluminationCorrection
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT IlluminationCorrection : public IFilter
{
public:
  IlluminationCorrection() = default;
  ~IlluminationCorrection() noexcept override = default;

  IlluminationCorrection(const IlluminationCorrection&) = delete;
  IlluminationCorrection(IlluminationCorrection&&) noexcept = delete;

  IlluminationCorrection& operator=(const IlluminationCorrection&) = delete;
  IlluminationCorrection& operator=(IlluminationCorrection&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MontageSelection_Key = "MontageSelection";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "CellAttributeMatrixName";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "ImageDataArrayName";
  static inline constexpr StringLiteral k_CorrectedImageDataArrayName_Key = "CorrectedImageDataArrayName";
  static inline constexpr StringLiteral k_BackgroundDataContainerPath_Key = "BackgroundDataContainerPath";
  static inline constexpr StringLiteral k_BackgroundCellAttributeMatrixPath_Key = "BackgroundCellAttributeMatrixPath";
  static inline constexpr StringLiteral k_BackgroundImageArrayPath_Key = "BackgroundImageArrayPath";
  static inline constexpr StringLiteral k_LowThreshold_Key = "LowThreshold";
  static inline constexpr StringLiteral k_HighThreshold_Key = "HighThreshold";
  static inline constexpr StringLiteral k_ApplyMedianFilter_Key = "ApplyMedianFilter";
  static inline constexpr StringLiteral k_MedianRadius_Key = "MedianRadius";
  static inline constexpr StringLiteral k_ApplyCorrection_Key = "ApplyCorrection";
  static inline constexpr StringLiteral k_ExportCorrectedImages_Key = "ExportCorrectedImages";
  static inline constexpr StringLiteral k_OutputPath_Key = "OutputPath";
  static inline constexpr StringLiteral k_FileExtension_Key = "FileExtension";
  static inline constexpr StringLiteral k_SelectedImageGeomPath_Key = "SelectedImageGeomPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, IlluminationCorrection, "95866cba-a92e-4d87-b7f6-8f03928ee633");
