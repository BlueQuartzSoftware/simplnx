#include "WriteASCIIDataFilter.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
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
std::string WriteASCIIDataFilter::name() const
{
  return FilterTraits<WriteASCIIDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteASCIIDataFilter::className() const
{
  return FilterTraits<WriteASCIIDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteASCIIDataFilter::uuid() const
{
  return FilterTraits<WriteASCIIDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteASCIIDataFilter::humanName() const
{
  return "Export ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteASCIIDataFilter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export", "#Text", "#CSV", "#ASCII"};
}

//------------------------------------------------------------------------------
Parameters WriteASCIIDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputStyle_Key, "Output Type", "", to_underlying(OutputStyle::MultipleFiles),
                                                                    ChoicesParameter::Choices{"Multiple Files", "Single File"})); // sequence dependent DO NOT REORDER
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileName_Key, "Name of Output File", "", "Data"));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "", ".csv"));
  params.insert(std::make_unique<Int32Parameter>(k_MaxValPerLine_Key, "Maximum Elements Per Line", "", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "Default Delimiter is Comma", to_underlying(OStreamUtilities::Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<ChoicesParameter>(k_Includes_Key, "Header and Index Options", "Default Include is Headers only", to_underlying(Includes::Headers),
                                                   ChoicesParameter::Choices{"Neither", "Headers", "Index", "Both"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "", MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_FileName_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));
  params.linkParameters(k_OutputStyle_Key, k_Includes_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteASCIIDataFilter::clone() const
{
  return std::make_unique<WriteASCIIDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteASCIIDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pMaxValPerLineValue = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  /////////////////////////////////////////////////////////////////////////////
  // VALIDATE THAT ALL DATA ARRAYS HAVE THE SAME NUMBER OF TUPLES.
  if(static_cast<WriteASCIIDataFilter::OutputStyle>(pOutputStyleValue) == WriteASCIIDataFilter::OutputStyle::SingleFile)
  {
    if(!CheckArraysHaveSameTupleCount(dataStructure, pSelectedDataArrayPathsValue))
    {
      return {MakeErrorResult<OutputActions>(k_UnmatchingTupleCountError, fmt::format("Arrays do not all have the same length, a requirement for single file."))};
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteASCIIDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  const auto includes = static_cast<WriteASCIIDataFilter::Includes>(filterArgs.value<ChoicesParameter::ValueType>(k_Includes_Key));
  bool includeHeaders = false;
  bool includeIndex = false;
  switch(includes)
  {
  case WriteASCIIDataFilter::Includes::Neither: // 0
  {
    includeHeaders = false;
    includeIndex = false;
    break;
  }
  case WriteASCIIDataFilter::Includes::Headers: // 1
  {
    includeHeaders = true;
    includeIndex = false;
    break;
  }
  case WriteASCIIDataFilter::Includes::ColumnIndex: // 2
  {
    includeHeaders = false;
    includeIndex = true;
    break;
  }
  case WriteASCIIDataFilter::Includes::Both: // 3
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
  int32 maxValPerLine = filterArgs.value<int32>(k_MaxValPerLine_Key);
  auto selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto fileType = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  auto directoryPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto fileName = filterArgs.value<StringParameter::ValueType>(k_FileName_Key);
  auto fileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  if(static_cast<WriteASCIIDataFilter::OutputStyle>(fileType) == WriteASCIIDataFilter::OutputStyle::SingleFile)
  {
    // Ensure the complete path to the output file exists or can be created
    if(!fs::exists(directoryPath.parent_path()))
    {
      if(!fs::create_directories(directoryPath.parent_path()))
      {
        return MakeErrorResult(-11020, fmt::format("Unable to create output directory {}", directoryPath.parent_path().string()));
      }
    }

    auto outputFilePath = fmt::format("{}/{}{}", directoryPath.string(), fileName, fileExtension);

    // Create the output file
    std::ofstream outStrm(outputFilePath, std::ios_base::out);
    if(!outStrm.is_open())
    {
      return MakeErrorResult(-11021, fmt::format("Unable to create output file {}", outputFilePath));
    }

    OStreamUtilities::PrintDataSetsToSingleFile(outStrm, selectedDataArrayPaths, dataStructure, messageHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
  }

  if(static_cast<WriteASCIIDataFilter::OutputStyle>(fileType) == WriteASCIIDataFilter::OutputStyle::MultipleFiles)
  {
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
