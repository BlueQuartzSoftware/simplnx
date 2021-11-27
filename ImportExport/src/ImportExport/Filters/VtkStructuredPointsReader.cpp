#include "VtkStructuredPointsReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VtkStructuredPointsReader::name() const
{
  return FilterTraits<VtkStructuredPointsReader>::name.str();
}

//------------------------------------------------------------------------------
std::string VtkStructuredPointsReader::className() const
{
  return FilterTraits<VtkStructuredPointsReader>::className;
}

//------------------------------------------------------------------------------
Uuid VtkStructuredPointsReader::uuid() const
{
  return FilterTraits<VtkStructuredPointsReader>::uuid;
}

//------------------------------------------------------------------------------
std::string VtkStructuredPointsReader::humanName() const
{
  return "VTK STRUCTURED_POINTS Importer";
}

//------------------------------------------------------------------------------
std::vector<std::string> VtkStructuredPointsReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters VtkStructuredPointsReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input VTK File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadPointData_Key, "Read Point Data", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadCellData_Key, "Read Cell Data", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexDataContainerName_Key, "Data Container [Point Data]", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VolumeDataContainerName_Key, "Data Container [Cell Data]", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Attribute Matrix [Point Data]", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Attribute Matrix [Cell Data]", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReadPointData_Key, k_VertexDataContainerName_Key, true);
  params.linkParameters(k_ReadPointData_Key, k_VertexAttributeMatrixName_Key, true);
  params.linkParameters(k_ReadCellData_Key, k_VolumeDataContainerName_Key, true);
  params.linkParameters(k_ReadCellData_Key, k_CellAttributeMatrixName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VtkStructuredPointsReader::clone() const
{
  return std::make_unique<VtkStructuredPointsReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VtkStructuredPointsReader::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pReadPointDataValue = filterArgs.value<bool>(k_ReadPointData_Key);
  auto pReadCellDataValue = filterArgs.value<bool>(k_ReadCellData_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);

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
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pVertexDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pVolumeDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> VtkStructuredPointsReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pReadPointDataValue = filterArgs.value<bool>(k_ReadPointData_Key);
  auto pReadCellDataValue = filterArgs.value<bool>(k_ReadCellData_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pVolumeDataContainerNameValue = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
