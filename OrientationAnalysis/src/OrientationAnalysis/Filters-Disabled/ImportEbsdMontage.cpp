#include "ImportEbsdMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/EbsdMontageImportFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportEbsdMontage::name() const
{
  return FilterTraits<ImportEbsdMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportEbsdMontage::className() const
{
  return FilterTraits<ImportEbsdMontage>::className;
}

//------------------------------------------------------------------------------
Uuid ImportEbsdMontage::uuid() const
{
  return FilterTraits<ImportEbsdMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportEbsdMontage::humanName() const
{
  return "Import EBSD Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportEbsdMontage::defaultTags() const
{
  return {"#Unsupported", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportEbsdMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<EbsdMontageImportFilterParameter>(k_InputFileListInfo_Key, "Input File List", "", {}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix Name", "", "SomeString"));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_DefineScanOverlap_Key, "Define Type of Scan Overlap", "", 0, ChoicesParameter::Choices{"None", "Pixel Overlap", "Percent Overlap"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ScanOverlapPixel_Key, "Pixel Overlap Value (X, Y)", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ScanOverlapPercent_Key, "Percent Overlap Value (X, Y)", "", std::vector<float32>(2), std::vector<std::string>(2)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_GenerateIPFColorMap_Key, "Generate IPF Color Map", "", false));
  params.insert(std::make_unique<StringParameter>(k_CellIPFColorsArrayName_Key, "IPF Colors", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DefineScanOverlap_Key, k_ScanOverlapPixel_Key, 0);
  params.linkParameters(k_DefineScanOverlap_Key, k_ScanOverlapPercent_Key, 1);
  params.linkParameters(k_GenerateIPFColorMap_Key, k_CellIPFColorsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportEbsdMontage::clone() const
{
  return std::make_unique<ImportEbsdMontage>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ImportEbsdMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileListInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellEnsembleAttributeMatrixName_Key);
  auto pDefineScanOverlapValue = filterArgs.value<ChoicesParameter::ValueType>(k_DefineScanOverlap_Key);
  auto pScanOverlapPixelValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ScanOverlapPixel_Key);
  auto pScanOverlapPercentValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScanOverlapPercent_Key);
  auto pGenerateIPFColorMapValue = filterArgs.value<bool>(k_GenerateIPFColorMap_Key);
  auto pCellIPFColorsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CellIPFColorsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportEbsdMontageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ImportEbsdMontage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileListInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellEnsembleAttributeMatrixName_Key);
  auto pDefineScanOverlapValue = filterArgs.value<ChoicesParameter::ValueType>(k_DefineScanOverlap_Key);
  auto pScanOverlapPixelValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ScanOverlapPixel_Key);
  auto pScanOverlapPercentValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScanOverlapPercent_Key);
  auto pGenerateIPFColorMapValue = filterArgs.value<bool>(k_GenerateIPFColorMap_Key);
  auto pCellIPFColorsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CellIPFColorsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
