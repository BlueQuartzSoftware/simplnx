#include "ImportDeformV12Filter.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/OStreamUtilities.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
// Error Code constants
constexpr complex::int32 k_UnmatchingTupleCountError = -51001;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportDeformV12Filter::name() const
{
  return FilterTraits<ImportDeformV12Filter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportDeformV12Filter::className() const
{
  return FilterTraits<ImportDeformV12Filter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDeformV12Filter::uuid() const
{
  return FilterTraits<ImportDeformV12Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDeformV12Filter::humanName() const
{
  return "Import Deform v12";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDeformV12Filter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ImportDeformV12Filter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputStyle_Key, "Output Type", "Whether to output a folder of files or a single file with all the data in column form",
                                                                    to_underlying(OutputStyle::MultipleFiles),
                                                                    ChoicesParameter::Choices{"Multiple Files", "Single File"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile, true));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputDir_Key, "Output Directory", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "The file extension for the output file(s)", ".txt"));
  params.insert(std::make_unique<Int32Parameter>(k_MaxValPerLine_Key, "Maximum Elements Per Line", "Number of tuples to print on each line", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "The delimiter separating the data", to_underlying(OStreamUtilities::Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<ChoicesParameter>(k_Includes_Key, "Header and Index Options", "Default Include is Headers only", to_underlying(Includes::Headers),
                                                   ChoicesParameter::Choices{"Neither", "Headers", "Index", "Both"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "Output arrays to be written as ASCII representations",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               complex::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_OutputDir_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));

  params.linkParameters(k_OutputStyle_Key, k_Includes_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));
  params.linkParameters(k_OutputStyle_Key, k_OutputPath_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportDeformV12Filter::clone() const
{
  return std::make_unique<ImportDeformV12Filter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportDeformV12Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  /////////////////////////////////////////////////////////////////////////////
  // VALIDATE THAT ALL DATA ARRAYS HAVE THE SAME NUMBER OF TUPLES.
  if(static_cast<ImportDeformV12Filter::OutputStyle>(pOutputStyleValue) == ImportDeformV12Filter::OutputStyle::SingleFile)
  {
    auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

    if(!CheckArraysHaveSameTupleCount(dataStructure, pSelectedDataArrayPathsValue))
    {
      return MakePreflightErrorResult(k_UnmatchingTupleCountError, "Arrays do not all have the same length, a requirement for single file.");
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportDeformV12Filter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  const auto includes = static_cast<ImportDeformV12Filter::Includes>(filterArgs.value<ChoicesParameter::ValueType>(k_Includes_Key));
  bool includeHeaders;
  bool includeIndex;
  switch(includes)
  {
  case ImportDeformV12Filter::Includes::Neither: // 0
  {
    includeHeaders = false;
    includeIndex = false;
    break;
  }
  case ImportDeformV12Filter::Includes::Headers: // 1
  {
    includeHeaders = true;
    includeIndex = false;
    break;
  }
  case ImportDeformV12Filter::Includes::ColumnIndex: // 2
  {
    includeHeaders = false;
    includeIndex = true;
    break;
  }
  case ImportDeformV12Filter::Includes::Both: // 3
  {
    includeHeaders = true;
    includeIndex = true;
    break;
  }
  default: {
    includeHeaders = false;
    includeIndex = false;
  }
  }

  const std::string delimiter = OStreamUtilities::DelimiterToString(filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key));
  auto selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto fileType = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);

  if(static_cast<ImportDeformV12Filter::OutputStyle>(fileType) == ImportDeformV12Filter::OutputStyle::SingleFile)
  {
    auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = complex::CreateOutputDirectories(outputPath.parent_path());
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }

    // Create the output file
    std::ofstream outStrm(outputPath, std::ios_base::out | std::ios_base::binary);
    if(!outStrm.is_open())
    {
      return MakeErrorResult(-11021, fmt::format("Unable to create output file {}", outputPath.string()));
    }

    OStreamUtilities::PrintDataSetsToSingleFile(outStrm, selectedDataArrayPaths, dataStructure, messageHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
  }

  if(static_cast<ImportDeformV12Filter::OutputStyle>(fileType) == ImportDeformV12Filter::OutputStyle::MultipleFiles)
  {
    auto directoryPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDir_Key);
    auto fileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
    auto maxValPerLine = filterArgs.value<int32>(k_MaxValPerLine_Key);

    if(!fs::exists(directoryPath))
    {
      if(!fs::create_directories(directoryPath))
      {
        return MakeErrorResult(-11022, fmt::format("Unable to create output directory {}", directoryPath.string()));
      }
    }
    OStreamUtilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, dataStructure, directoryPath.string(), messageHandler, shouldCancel, fileExtension, false, delimiter, includeIndex,
                                                   includeHeaders, maxValPerLine);
  }
  return {};
}
} // namespace complex
