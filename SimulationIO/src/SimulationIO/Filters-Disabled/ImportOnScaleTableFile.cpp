#include "ImportOnScaleTableFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ImportOnScaleTableFile::name() const
{
  return FilterTraits<ImportOnScaleTableFile>::name.str();
}

std::string ImportOnScaleTableFile::className() const
{
  return FilterTraits<ImportOnScaleTableFile>::className;
}

Uuid ImportOnScaleTableFile::uuid() const
{
  return FilterTraits<ImportOnScaleTableFile>::uuid;
}

std::string ImportOnScaleTableFile::humanName() const
{
  return "Import Onscale Table File (.flxtbl)";
}

Parameters ImportOnScaleTableFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Fallback Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Fallback Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_RectGridGeometryDesc_Key, "Rect Grid Geom Info.", "", {}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VolumeDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Cell Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Material Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhaseAttributeMatrixName_Key, "Phase Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MaterialNameArrayName_Key, "Material Names", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ImportOnScaleTableFile::clone() const
{
  return std::make_unique<ImportOnScaleTableFile>();
}

Result<OutputActions> ImportOnScaleTableFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pRectGridGeometryDescValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pPhaseAttributeMatrixNameValue = filterArgs.value<DataPath>(k_PhaseAttributeMatrixName_Key);
  auto pMaterialNameArrayNameValue = filterArgs.value<DataPath>(k_MaterialNameArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportOnScaleTableFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportOnScaleTableFile::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pRectGridGeometryDescValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pPhaseAttributeMatrixNameValue = filterArgs.value<DataPath>(k_PhaseAttributeMatrixName_Key);
  auto pMaterialNameArrayNameValue = filterArgs.value<DataPath>(k_MaterialNameArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
