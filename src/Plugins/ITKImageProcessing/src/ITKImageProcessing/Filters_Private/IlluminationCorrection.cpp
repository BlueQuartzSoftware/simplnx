#include "IlluminationCorrection.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
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

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkIlluminationCorrectionFilter.h>
namespace
{
struct ITKIlluminationCorrectionFilterCreationFunctor
{
  template <class InputImageType, class OutputImageType>
  auto operator()() const
  {
    using FilterType = itk::IlluminationCorrectionFilter<InputImageType, OutputImageType>;
    FilterType filter = FilterType::New();
    return filter;
  }
};
} // namespace

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
  return "ITK Illumination Correction";
}

//------------------------------------------------------------------------------
std::vector<std::string> IlluminationCorrection::defaultTags() const
{
  return {"Processing", "Image"};
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
  params.insert(std::make_unique<DataGroupCreationParameter>(k_BackgroundDataContainerPath_Key, "Created Data Container (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_BackgroundCellAttributeMatrixPath_Key, "Created Attribute Matrix (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_BackgroundImageArrayPath_Key, "Created Image Array Name (Corrected)", "", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_LowThreshold_Key, "Lowest allowed Image value (Image Value)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_HighThreshold_Key, "Highest allowed Image value (Image Value)", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyMedianFilter_Key, "Apply median filter to background image", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MedianRadius_Key, "MedianRadius", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyCorrection_Key, "Apply Background Correction to Input Images", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ExportCorrectedImages_Key, "Export Corrected Images", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
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
IFilter::PreflightResult IlluminationCorrection::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
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
  auto pMedianRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MedianRadius_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  PreflightResult preflightResult;
  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions;

  resultOutputActions = ITK::DataCheck(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath);

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pBackgroundDataContainerPathValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pBackgroundCellAttributeMatrixPathValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pBackgroundImageArrayPathValue);
    // resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> IlluminationCorrection::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
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
  auto pMedianRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MedianRadius_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, IlluminationCorrectionFilterCreationFunctor{});
}
} // namespace complex
