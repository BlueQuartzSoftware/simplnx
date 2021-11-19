#include "ImportDelamData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportDelamData::name() const
{
  return FilterTraits<ImportDelamData>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportDelamData::className() const
{
  return FilterTraits<ImportDelamData>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDelamData::uuid() const
{
  return FilterTraits<ImportDelamData>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDelamData::humanName() const
{
  return "Import Delam Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDelamData::defaultTags() const
{
  return {"#IO", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ImportDelamData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_CSDGMFile_Key, "CSDGM File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_BvidStdOutFile_Key, "Bvid StdOut File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<Float32Parameter>(k_InterfaceThickness_Key, "Interface Thickness", "", 1.23345f));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "Data Container", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_DataArrayName_Key, "Data Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportDelamData::clone() const
{
  return std::make_unique<ImportDelamData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportDelamData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCSDGMFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CSDGMFile_Key);
  auto pBvidStdOutFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BvidStdOutFile_Key);
  auto pInterfaceThicknessValue = filterArgs.value<float32>(k_InterfaceThickness_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayName_Key);

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

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportDelamData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCSDGMFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CSDGMFile_Key);
  auto pBvidStdOutFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BvidStdOutFile_Key);
  auto pInterfaceThicknessValue = filterArgs.value<float32>(k_InterfaceThickness_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
