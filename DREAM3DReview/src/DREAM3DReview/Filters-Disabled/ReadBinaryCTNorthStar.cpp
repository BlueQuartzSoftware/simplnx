#include "ReadBinaryCTNorthStar.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ReadBinaryCTNorthStar::name() const
{
  return FilterTraits<ReadBinaryCTNorthStar>::name.str();
}

Uuid ReadBinaryCTNorthStar::uuid() const
{
  return FilterTraits<ReadBinaryCTNorthStar>::uuid;
}

std::string ReadBinaryCTNorthStar::humanName() const
{
  return "Import North Star Imaging CT (.nsihdr/.nsidat)";
}

Parameters ReadBinaryCTNorthStar::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputHeaderFile_Key, "Input Header File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_DataContainerName_Key, "Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DensityArrayName_Key, "Density", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_VolumeDescription_Key, "Original Volume", "", {}));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_DataFileInfo_Key, "Dat Files", "", {}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ImportSubvolume_Key, "Import Subvolume", "", false));
  params.insert(std::make_unique<VectorInt32Parameter>(k_StartVoxelCoord_Key, "Starting XYZ Voxel", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_EndVoxelCoord_Key, "Ending XYZ Voxel", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_ImportedVolumeDescription_Key, "Imported Volume", "", {}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ImportSubvolume_Key, k_StartVoxelCoord_Key, true);
  params.linkParameters(k_ImportSubvolume_Key, k_EndVoxelCoord_Key, true);
  params.linkParameters(k_ImportSubvolume_Key, k_ImportedVolumeDescription_Key, true);

  return params;
}

IFilter::UniquePointer ReadBinaryCTNorthStar::clone() const
{
  return std::make_unique<ReadBinaryCTNorthStar>();
}

Result<OutputActions> ReadBinaryCTNorthStar::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pVolumeDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_VolumeDescription_Key);
  auto pDataFileInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataFileInfo_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);
  auto pImportedVolumeDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportedVolumeDescription_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReadBinaryCTNorthStarAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ReadBinaryCTNorthStar::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pVolumeDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_VolumeDescription_Key);
  auto pDataFileInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataFileInfo_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);
  auto pImportedVolumeDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportedVolumeDescription_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
