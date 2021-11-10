#include "ITKImportRoboMetMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKImportRoboMetMontage::name() const
{
  return FilterTraits<ITKImportRoboMetMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKImportRoboMetMontage::className() const
{
  return FilterTraits<ITKImportRoboMetMontage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImportRoboMetMontage::uuid() const
{
  return FilterTraits<ITKImportRoboMetMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImportRoboMetMontage::humanName() const
{
  return "ITK::Import RoboMet Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportRoboMetMontage::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportRoboMetMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Registration File (Mosaic Details)", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Int32Parameter>(k_SliceNumber_Key, "Slice Number", "", 1234356));
  params.insert(std::make_unique<StringParameter>(k_ImageFilePrefix_Key, "Image File Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ImageFileExtension_Key, "Image File Extension", "", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Change Spacing", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "DataContainer Prefix", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImportRoboMetMontage::clone() const
{
  return std::make_unique<ITKImportRoboMetMontage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImportRoboMetMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pSliceNumberValue = filterArgs.value<int32>(k_SliceNumber_Key);
  auto pImageFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_ImageFilePrefix_Key);
  auto pImageFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_ImageFileExtension_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pChangeSpacingValue = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string montageInformation;

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKImportRoboMetMontageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  // These values should have been updated during the preflightImpl(...) method
  preflightResult.outputValues.push_back({"MontageInformation", montageInformation});

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKImportRoboMetMontage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pSliceNumberValue = filterArgs.value<int32>(k_SliceNumber_Key);
  auto pImageFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_ImageFilePrefix_Key);
  auto pImageFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_ImageFileExtension_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pChangeSpacingValue = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
