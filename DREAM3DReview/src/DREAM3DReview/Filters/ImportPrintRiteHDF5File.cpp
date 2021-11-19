#include "ImportPrintRiteHDF5File.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportPrintRiteHDF5File::name() const
{
  return FilterTraits<ImportPrintRiteHDF5File>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportPrintRiteHDF5File::className() const
{
  return FilterTraits<ImportPrintRiteHDF5File>::className;
}

//------------------------------------------------------------------------------
Uuid ImportPrintRiteHDF5File::uuid() const
{
  return FilterTraits<ImportPrintRiteHDF5File>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportPrintRiteHDF5File::humanName() const
{
  return "Import PrintRite HDF5 File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportPrintRiteHDF5File::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportPrintRiteHDF5File::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input TDMS-HDF5 File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_HFDataContainerName_Key, "High Frequency Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_HFDataName_Key, "High Frequency Vertex Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_HFSliceIdsArrayName_Key, "High Frequency Slice Ids", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Feature Data"});
  params.insert(std::make_unique<StringParameter>(k_HFSliceDataName_Key, "High Frequency Slice Attribute Matrix", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportPrintRiteHDF5File::clone() const
{
  return std::make_unique<ImportPrintRiteHDF5File>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportPrintRiteHDF5File::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pHFDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataContainerName_Key);
  auto pHFDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataName_Key);
  auto pHFSliceIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceIdsArrayName_Key);
  auto pHFSliceDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceDataName_Key);

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
Result<> ImportPrintRiteHDF5File::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pHFDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataContainerName_Key);
  auto pHFDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataName_Key);
  auto pHFSliceIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceIdsArrayName_Key);
  auto pHFSliceDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceDataName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
