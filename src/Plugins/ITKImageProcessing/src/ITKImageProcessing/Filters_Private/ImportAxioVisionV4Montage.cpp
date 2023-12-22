#include "ImportAxioVisionV4Montage.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ImportAxioVisionV4Montage::name() const
{
  return FilterTraits<ImportAxioVisionV4Montage>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportAxioVisionV4Montage::className() const
{
  return FilterTraits<ImportAxioVisionV4Montage>::className;
}

//------------------------------------------------------------------------------
Uuid ImportAxioVisionV4Montage::uuid() const
{
  return FilterTraits<ImportAxioVisionV4Montage>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportAxioVisionV4Montage::humanName() const
{
  return "ITK Zeiss AxioVision Import (V4)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportAxioVisionV4Montage::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ImportAxioVisionV4Montage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "AxioVision XML File (_meta.xml)", "", fs::path("<default file to read goes here>"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<BoolParameter>(k_ImportAllMetaData_Key, "Import All MetaData", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Change Spacing", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "DataContainer Prefix", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MetaDataAttributeMatrixName_Key, "MetaData AttributeMatrix Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportAxioVisionV4Montage::clone() const
{
  return std::make_unique<ImportAxioVisionV4Montage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportAxioVisionV4Montage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pImportAllMetaDataValue = filterArgs.value<bool>(k_ImportAllMetaData_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pChangeSpacingValue = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto pMetaDataAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MetaDataAttributeMatrixName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string montageInformation;

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in simplnx/Filter/Actions
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
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerPathValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"MontageInformation", montageInformation});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportAxioVisionV4Montage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pImportAllMetaDataValue = filterArgs.value<bool>(k_ImportAllMetaData_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pChangeSpacingValue = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto pMetaDataAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MetaDataAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace nx::core
