#include "ImportPrintRiteTDMSFiles.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportPrintRiteTDMSFiles::name() const
{
  return FilterTraits<ImportPrintRiteTDMSFiles>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportPrintRiteTDMSFiles::className() const
{
  return FilterTraits<ImportPrintRiteTDMSFiles>::className;
}

//------------------------------------------------------------------------------
Uuid ImportPrintRiteTDMSFiles::uuid() const
{
  return FilterTraits<ImportPrintRiteTDMSFiles>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportPrintRiteTDMSFiles::humanName() const
{
  return "Import PrintRite TDMS File(s) to HDF5";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportPrintRiteTDMSFiles::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportPrintRiteTDMSFiles::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Build Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_LayerThickness_Key, "Layer Thickness", "", 1.23345f));
  params.insert(std::make_unique<ChoicesParameter>(k_LaserOnArrayOption_Key, "Array Used to Determine if Laser is On", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Float32Parameter>(k_LaserOnThreshold_Key, "Laser On Threshold", "", 1.23345f));
  params.insert(std::make_unique<BoolParameter>(k_DowncastRawData_Key, "Downcast Raw Data", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ScaleLaserPower_Key, "Scale Laser Power", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_PowerScalingCoefficients_Key, "Power Scaling Coefficients", "", std::vector<float32>(2), std::vector<std::string>(2)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ScalePyrometerTemperature_Key, "Scale Pyrometer Tempeature", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_TemperatureScalingCoefficients_Key, "Temperature Scaling Coefficients", "", std::vector<float32>(2), std::vector<std::string>(2)));
  params.insertSeparator(Parameters::Separator{"Spatial Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_SpatialTransformOption_Key, "Spatial Transform Options", "", 0,
      ChoicesParameter::Choices{"Do Not Compute Real Space Transformation", "Compute Real Space Transformation", "Import Spatial Transformation Coefficients From File"}));
  params.insert(std::make_unique<Int32Parameter>(k_LayerForScaling_Key, "Layer Index For Computing Real Space Transformation", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_SearchRadius_Key, "Search Radius for Finding Regions", "", 1.23345f));
  params.insert(std::make_unique<BoolParameter>(k_SplitRegions1_Key, "Split Contiguous Regions into Separate Files", "", false));
  params.insert(std::make_unique<BoolParameter>(k_SplitRegions2_Key, "Split Contiguous Regions into Separate Files", "", false));
  params.insertSeparator(Parameters::Separator{"Input File Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_STLFilePath1_Key, "Build STL File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_STLFilePath2_Key, "Build STL File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputSpatialTransformFilePath_Key, "Spatial Transformation Coefficients File", "", fs::path("<default file to read goes here>"),
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_InputFilesList_Key, "Input PrintRite Files", "", GeneratedFileListParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Output File Parameters"});
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputDirectory_Key, "Output File Directory", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputFilePrefix_Key, "Output File Prefix", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ScaleLaserPower_Key, k_PowerScalingCoefficients_Key, true);
  params.linkParameters(k_ScalePyrometerTemperature_Key, k_TemperatureScalingCoefficients_Key, true);
  params.linkParameters(k_SpatialTransformOption_Key, k_LayerForScaling_Key, 1);
  params.linkParameters(k_SpatialTransformOption_Key, k_SearchRadius_Key, 1);
  params.linkParameters(k_SpatialTransformOption_Key, k_SplitRegions1_Key, 1);
  params.linkParameters(k_SpatialTransformOption_Key, k_SplitRegions2_Key, 2);
  params.linkParameters(k_SpatialTransformOption_Key, k_STLFilePath1_Key, 1);
  params.linkParameters(k_SpatialTransformOption_Key, k_STLFilePath2_Key, 2);
  params.linkParameters(k_SpatialTransformOption_Key, k_InputSpatialTransformFilePath_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportPrintRiteTDMSFiles::clone() const
{
  return std::make_unique<ImportPrintRiteTDMSFiles>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportPrintRiteTDMSFiles::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pLayerThicknessValue = filterArgs.value<float32>(k_LayerThickness_Key);
  auto pLaserOnArrayOptionValue = filterArgs.value<ChoicesParameter::ValueType>(k_LaserOnArrayOption_Key);
  auto pLaserOnThresholdValue = filterArgs.value<float32>(k_LaserOnThreshold_Key);
  auto pDowncastRawDataValue = filterArgs.value<bool>(k_DowncastRawData_Key);
  auto pScaleLaserPowerValue = filterArgs.value<bool>(k_ScaleLaserPower_Key);
  auto pPowerScalingCoefficientsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PowerScalingCoefficients_Key);
  auto pScalePyrometerTemperatureValue = filterArgs.value<bool>(k_ScalePyrometerTemperature_Key);
  auto pTemperatureScalingCoefficientsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TemperatureScalingCoefficients_Key);
  auto pSpatialTransformOptionValue = filterArgs.value<ChoicesParameter::ValueType>(k_SpatialTransformOption_Key);
  auto pLayerForScalingValue = filterArgs.value<int32>(k_LayerForScaling_Key);
  auto pSearchRadiusValue = filterArgs.value<float32>(k_SearchRadius_Key);
  auto pSplitRegions1Value = filterArgs.value<bool>(k_SplitRegions1_Key);
  auto pSplitRegions2Value = filterArgs.value<bool>(k_SplitRegions2_Key);
  auto pSTLFilePath1Value = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath1_Key);
  auto pSTLFilePath2Value = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath2_Key);
  auto pInputSpatialTransformFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputSpatialTransformFilePath_Key);
  auto pInputFilesListValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFilesList_Key);
  auto pOutputDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);

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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportPrintRiteTDMSFiles::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLayerThicknessValue = filterArgs.value<float32>(k_LayerThickness_Key);
  auto pLaserOnArrayOptionValue = filterArgs.value<ChoicesParameter::ValueType>(k_LaserOnArrayOption_Key);
  auto pLaserOnThresholdValue = filterArgs.value<float32>(k_LaserOnThreshold_Key);
  auto pDowncastRawDataValue = filterArgs.value<bool>(k_DowncastRawData_Key);
  auto pScaleLaserPowerValue = filterArgs.value<bool>(k_ScaleLaserPower_Key);
  auto pPowerScalingCoefficientsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PowerScalingCoefficients_Key);
  auto pScalePyrometerTemperatureValue = filterArgs.value<bool>(k_ScalePyrometerTemperature_Key);
  auto pTemperatureScalingCoefficientsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TemperatureScalingCoefficients_Key);
  auto pSpatialTransformOptionValue = filterArgs.value<ChoicesParameter::ValueType>(k_SpatialTransformOption_Key);
  auto pLayerForScalingValue = filterArgs.value<int32>(k_LayerForScaling_Key);
  auto pSearchRadiusValue = filterArgs.value<float32>(k_SearchRadius_Key);
  auto pSplitRegions1Value = filterArgs.value<bool>(k_SplitRegions1_Key);
  auto pSplitRegions2Value = filterArgs.value<bool>(k_SplitRegions2_Key);
  auto pSTLFilePath1Value = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath1_Key);
  auto pSTLFilePath2Value = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath2_Key);
  auto pInputSpatialTransformFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputSpatialTransformFilePath_Key);
  auto pInputFilesListValue = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFilesList_Key);
  auto pOutputDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
