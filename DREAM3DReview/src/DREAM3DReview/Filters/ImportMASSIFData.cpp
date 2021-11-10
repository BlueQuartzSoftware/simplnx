#include "ImportMASSIFData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportMASSIFData::name() const
{
  return FilterTraits<ImportMASSIFData>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportMASSIFData::className() const
{
  return FilterTraits<ImportMASSIFData>::className;
}

//------------------------------------------------------------------------------
Uuid ImportMASSIFData::uuid() const
{
  return FilterTraits<ImportMASSIFData>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportMASSIFData::humanName() const
{
  return "Import MASSIF Data (HDF5)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportMASSIFData::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportMASSIFData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_MassifInputFilePath_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "File Prefix", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_StepNumber_Key, "Step Value", "", 1234356));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportMASSIFData::clone() const
{
  return std::make_unique<ImportMASSIFData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportMASSIFData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMassifInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_MassifInputFilePath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pStepNumberValue = filterArgs.value<int32>(k_StepNumber_Key);

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
  auto action = std::make_unique<ImportMASSIFDataAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ImportMASSIFData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMassifInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_MassifInputFilePath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pStepNumberValue = filterArgs.value<int32>(k_StepNumber_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
