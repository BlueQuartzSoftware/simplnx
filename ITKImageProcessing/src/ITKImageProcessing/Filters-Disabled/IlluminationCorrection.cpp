#include "IlluminationCorrection.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  /*[x]*/ params.insert(std::make_unique<MontageSelectionFilterParameter>(k_MontageSelection_Key, "Montage Selection", "", {}));
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
IFilter::PreflightResult IlluminationCorrection::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
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

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> IlluminationCorrection::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
