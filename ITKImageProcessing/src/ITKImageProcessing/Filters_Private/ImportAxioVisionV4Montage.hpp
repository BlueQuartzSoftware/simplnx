#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ImportAxioVisionV4Montage
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ImportAxioVisionV4Montage : public IFilter
{
public:
  ImportAxioVisionV4Montage() = default;
  ~ImportAxioVisionV4Montage() noexcept override = default;

  ImportAxioVisionV4Montage(const ImportAxioVisionV4Montage&) = delete;
  ImportAxioVisionV4Montage(ImportAxioVisionV4Montage&&) noexcept = delete;

  ImportAxioVisionV4Montage& operator=(const ImportAxioVisionV4Montage&) = delete;
  ImportAxioVisionV4Montage& operator=(ImportAxioVisionV4Montage&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "InputFile";
  static inline constexpr StringLiteral k_MontageName_Key = "MontageName";
  static inline constexpr StringLiteral k_ColumnMontageLimits_Key = "ColumnMontageLimits";
  static inline constexpr StringLiteral k_RowMontageLimits_Key = "RowMontageLimits";
  static inline constexpr StringLiteral k_ImportAllMetaData_Key = "ImportAllMetaData";
  static inline constexpr StringLiteral k_ChangeOrigin_Key = "ChangeOrigin";
  static inline constexpr StringLiteral k_Origin_Key = "Origin";
  static inline constexpr StringLiteral k_ChangeSpacing_Key = "ChangeSpacing";
  static inline constexpr StringLiteral k_Spacing_Key = "Spacing";
  static inline constexpr StringLiteral k_ConvertToGrayScale_Key = "ConvertToGrayScale";
  static inline constexpr StringLiteral k_ColorWeights_Key = "ColorWeights";
  static inline constexpr StringLiteral k_DataContainerPath_Key = "DataContainerPath";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "CellAttributeMatrixName";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "ImageDataArrayName";
  static inline constexpr StringLiteral k_MetaDataAttributeMatrixName_Key = "MetaDataAttributeMatrixName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ImportAxioVisionV4Montage, "697f306e-9acd-4aba-b2c9-486acda269d4");
