#include "ImportPrintRiteHDF5File.hpp"

#include "complex/DataStructure/DataPath.hpp"
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

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportPrintRiteHDF5FileAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
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
