#include "ImportAxioVisionV4Montage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
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
  return "ITK::Zeiss AxioVision Import (V4)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportAxioVisionV4Montage::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportAxioVisionV4Montage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "AxioVision XML File (_meta.xml)", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_MontageInformation_Key, "Montage Information", "", {}));
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
Result<OutputActions> ImportAxioVisionV4Montage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pMontageInformationValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageInformation_Key);
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportAxioVisionV4MontageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ImportAxioVisionV4Montage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pMontageInformationValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageInformation_Key);
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
} // namespace complex
