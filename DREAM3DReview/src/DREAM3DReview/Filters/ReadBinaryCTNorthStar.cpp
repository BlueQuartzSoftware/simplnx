#include "ReadBinaryCTNorthStar.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthStar::name() const
{
  return FilterTraits<ReadBinaryCTNorthStar>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthStar::className() const
{
  return FilterTraits<ReadBinaryCTNorthStar>::className;
}

//------------------------------------------------------------------------------
Uuid ReadBinaryCTNorthStar::uuid() const
{
  return FilterTraits<ReadBinaryCTNorthStar>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthStar::humanName() const
{
  return "Import North Star Imaging CT (.nsihdr/.nsidat)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadBinaryCTNorthStar::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
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
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ImportSubvolume_Key, "Import Subvolume", "", false));
  params.insert(std::make_unique<VectorInt32Parameter>(k_StartVoxelCoord_Key, "Starting XYZ Voxel", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_EndVoxelCoord_Key, "Ending XYZ Voxel", "", std::vector<int32>(3), std::vector<std::string>(3)));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ImportSubvolume_Key, k_StartVoxelCoord_Key, true);
  params.linkParameters(k_ImportSubvolume_Key, k_EndVoxelCoord_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadBinaryCTNorthStar::clone() const
{
  return std::make_unique<ReadBinaryCTNorthStar>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadBinaryCTNorthStar::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string volumeDescription;
  std::string dataFileInfo;
  std::string importedVolumeDescription;

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
  auto action = std::make_unique<ReadBinaryCTNorthStarAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  // These values should have been updated during the preflightImpl(...) method
  preflightResult.outputValues.push_back({"VolumeDescription", volumeDescription});
  preflightResult.outputValues.push_back({"DataFileInfo", dataFileInfo});
  preflightResult.outputValues.push_back({"ImportedVolumeDescription", importedVolumeDescription});

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ReadBinaryCTNorthStar::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
