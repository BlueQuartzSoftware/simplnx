#include "ReadNotesFileFilter.hpp"

#include "CopyDataObjectFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/ReadNotesFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadNotesFileFilter::name() const
{
  return FilterTraits<ReadNotesFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadNotesFileFilter::className() const
{
  return FilterTraits<ReadNotesFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadNotesFileFilter::uuid() const
{
  return FilterTraits<ReadNotesFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadNotesFileFilter::humanName() const
{
  return "Read Notes File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadNotesFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Notes", "Text"};
}

//------------------------------------------------------------------------------
Parameters ReadNotesFileFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input text file", fs::path("input.ang"),
                                                          FileSystemPathParameter::ExtensionsType{{".txt"}, {".md"}, {".rst"}, {".text"}, {".*"}}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayPath_Key, "Created Array Path", "DataPath or Name for the underlying Data Array", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadNotesFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadNotesFileFilter::clone() const
{
  return std::make_unique<ReadNotesFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadNotesFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                            const ExecutionContext& executionContext) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto arrayPath = filterArgs.value<DataPath>(k_DataArrayPath_Key);
  usize size_in_bytes = 0;
  try
  {
    // Check if the path points to a regular file
    if(fs::is_regular_file(pInputFileValue))
    {
      // Get the size of the file in bytes
      size_in_bytes = fs::file_size(pInputFileValue);
    }
    else
    {
      return {MakeErrorResult<OutputActions>(-38200, fmt::format("The path '{}' does not point to a regular file, or the file does not exist.", pInputFileValue.string()))};
    }
  } catch(fs::filesystem_error const& ex)
  {
    // Handle potential errors (e.g., file not found, permission denied)
    return {MakeErrorResult<OutputActions>(-38201, fmt::format("Filesystem error for file '{}'\nOperating System returned error '{}'", pInputFileValue.string(), ex.what()))};
  }

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  Result<OutputActions> resultOutputActions;
  ShapeType tupleDims = {1};
  resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(tupleDims, arrayPath));

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadNotesFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadNotesFileInputValues inputValues;

  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.CreatedDataPath = filterArgs.value<DataPath>(k_DataArrayPath_Key);

  return ReadNotesFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{

// TODO: PARAMETER_JSON_CONSTANTS
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> ReadNotesFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadNotesFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  /* This is a NEW filter and not ported so this section does not matter */

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
