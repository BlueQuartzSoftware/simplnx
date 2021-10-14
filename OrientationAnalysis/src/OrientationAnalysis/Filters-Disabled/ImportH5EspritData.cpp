#include "ImportH5EspritData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/OEMEbsdScanSelectionFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ImportH5EspritData::name() const
{
  return FilterTraits<ImportH5EspritData>::name.str();
}

std::string ImportH5EspritData::className() const
{
  return FilterTraits<ImportH5EspritData>::className;
}

Uuid ImportH5EspritData::uuid() const
{
  return FilterTraits<ImportH5EspritData>::uuid;
}

std::string ImportH5EspritData::humanName() const
{
  return "Import Bruker Nano Esprit Data (.h5)";
}

Parameters ImportH5EspritData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<OEMEbsdScanSelectionFilterParameter>(k_SelectedScanNames_Key, "Scan Names", "", {}));
  params.insert(std::make_unique<Float64Parameter>(k_ZSpacing_Key, "Z Spacing (Microns)", "", 2.3456789));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin (XYZ)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<BoolParameter>(k_CombineEulerAngles_Key, "Combine phi1, PHI, phi2 into Single Euler Angles Attribute Array", "", false));
  params.insert(std::make_unique<BoolParameter>(k_DegreesToRadians_Key, "Convert Euler Angles to Radians", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ReadPatternData_Key, "Import Pattern Data", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ImportH5EspritData::clone() const
{
  return std::make_unique<ImportH5EspritData>();
}

Result<OutputActions> ImportH5EspritData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pSelectedScanNamesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float64>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pCombineEulerAnglesValue = filterArgs.value<bool>(k_CombineEulerAngles_Key);
  auto pDegreesToRadiansValue = filterArgs.value<bool>(k_DegreesToRadians_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportH5EspritDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportH5EspritData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pSelectedScanNamesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedScanNames_Key);
  auto pZSpacingValue = filterArgs.value<float64>(k_ZSpacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pCombineEulerAnglesValue = filterArgs.value<bool>(k_CombineEulerAngles_Key);
  auto pDegreesToRadiansValue = filterArgs.value<bool>(k_DegreesToRadians_Key);
  auto pReadPatternDataValue = filterArgs.value<bool>(k_ReadPatternData_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
