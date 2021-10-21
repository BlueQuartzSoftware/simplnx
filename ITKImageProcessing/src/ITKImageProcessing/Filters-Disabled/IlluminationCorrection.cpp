#include "IlluminationCorrection.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MontageSelectionFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string IlluminationCorrection::name() const
{
  return FilterTraits<IlluminationCorrection>::name.str();
}

//------------------------------------------------------------------------------
std::string IlluminationCorrection::className() const
{
  return FilterTraits<IlluminationCorrection>::className;
}

//------------------------------------------------------------------------------
Uuid IlluminationCorrection::uuid() const
{
  return FilterTraits<IlluminationCorrection>::uuid;
}

//------------------------------------------------------------------------------
std::string IlluminationCorrection::humanName() const
{
  return "ITK::Illumination Correction";
}

//------------------------------------------------------------------------------
std::vector<std::string> IlluminationCorrection::defaultTags() const
{
  return {"#Processing", "#Image"};
}

//------------------------------------------------------------------------------
Parameters IlluminationCorrection::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<MontageSelectionFilterParameter>(k_MontageSelection_Key, "Montage Selection", "", {}));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Input Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Input Image Array Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CorrectedImageDataArrayName_Key, "Corrected Image Name", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Created Background Image Name"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_BackgroundDataContainerPath_Key, "Created Data Container (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_BackgroundCellAttributeMatrixPath_Key, "Created Attribute Matrix (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_BackgroundImageArrayPath_Key, "Created Image Array Name (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_LowThreshold_Key, "Lowest allowed Image value (Image Value)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_HighThreshold_Key, "Highest allowed Image value (Image Value)", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Background Image Processing"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyMedianFilter_Key, "Apply median filter to background image", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MedianRadius_Key, "MedianRadius", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Process Input Images"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyCorrection_Key, "Apply Background Correction to Input Images", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ExportCorrectedImages_Key, "Export Corrected Images", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ApplyMedianFilter_Key, k_MedianRadius_Key, true);
  params.linkParameters(k_ApplyCorrection_Key, k_CorrectedImageDataArrayName_Key, true);
  params.linkParameters(k_ApplyCorrection_Key, k_ExportCorrectedImages_Key, true);
  params.linkParameters(k_ExportCorrectedImages_Key, k_OutputPath_Key, true);
  params.linkParameters(k_ExportCorrectedImages_Key, k_FileExtension_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IlluminationCorrection::clone() const
{
  return std::make_unique<IlluminationCorrection>();
}

//------------------------------------------------------------------------------
Result<OutputActions> IlluminationCorrection::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMontageSelectionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto pCorrectedImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CorrectedImageDataArrayName_Key);
  auto pBackgroundDataContainerPathValue = filterArgs.value<DataPath>(k_BackgroundDataContainerPath_Key);
  auto pBackgroundCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_BackgroundCellAttributeMatrixPath_Key);
  auto pBackgroundImageArrayPathValue = filterArgs.value<DataPath>(k_BackgroundImageArrayPath_Key);
  auto pLowThresholdValue = filterArgs.value<int32>(k_LowThreshold_Key);
  auto pHighThresholdValue = filterArgs.value<int32>(k_HighThreshold_Key);
  auto pApplyMedianFilterValue = filterArgs.value<bool>(k_ApplyMedianFilter_Key);
  auto pMedianRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MedianRadius_Key);
  auto pApplyCorrectionValue = filterArgs.value<bool>(k_ApplyCorrection_Key);
  auto pExportCorrectedImagesValue = filterArgs.value<bool>(k_ExportCorrectedImages_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<IlluminationCorrectionAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> IlluminationCorrection::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageSelectionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto pCorrectedImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CorrectedImageDataArrayName_Key);
  auto pBackgroundDataContainerPathValue = filterArgs.value<DataPath>(k_BackgroundDataContainerPath_Key);
  auto pBackgroundCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_BackgroundCellAttributeMatrixPath_Key);
  auto pBackgroundImageArrayPathValue = filterArgs.value<DataPath>(k_BackgroundImageArrayPath_Key);
  auto pLowThresholdValue = filterArgs.value<int32>(k_LowThreshold_Key);
  auto pHighThresholdValue = filterArgs.value<int32>(k_HighThreshold_Key);
  auto pApplyMedianFilterValue = filterArgs.value<bool>(k_ApplyMedianFilter_Key);
  auto pMedianRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MedianRadius_Key);
  auto pApplyCorrectionValue = filterArgs.value<bool>(k_ApplyCorrection_Key);
  auto pExportCorrectedImagesValue = filterArgs.value<bool>(k_ExportCorrectedImages_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
