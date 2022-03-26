#include "ReadBinaryCTNorthStar.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputHeaderFile_Key, "Input Header File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
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
IFilter::PreflightResult ReadBinaryCTNorthStar::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);

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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string volumeDescription;
  std::string dataFileInfo;
  std::string importedVolumeDescription;

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
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"VolumeDescription", volumeDescription});
  preflightUpdatedValues.push_back({"DataFileInfo", dataFileInfo});
  preflightUpdatedValues.push_back({"ImportedVolumeDescription", importedVolumeDescription});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadBinaryCTNorthStar::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
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
