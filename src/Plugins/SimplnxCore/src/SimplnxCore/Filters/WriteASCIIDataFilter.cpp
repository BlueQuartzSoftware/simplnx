#include "WriteASCIIDataFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// Error Code constants
constexpr nx::core::int32 k_UnmatchingTupleCountError = -51001;
constexpr nx::core::int32 k_NoArraySelections = -51002;
} // namespace

namespace nx::core
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
  return "Write ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteASCIIDataFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Text", "CSV", "ASCII"};
}

//------------------------------------------------------------------------------
Parameters WriteASCIIDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputStyle_Key, "Output File Generation",
                                                                    "Whether to output a folder of files or a single file with all the data in column form", to_underlying(OutputStyle::SingleFile),
                                                                    ChoicesParameter::Choices{"Multiple Files", "Single File"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile, true));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputDir_Key, "Output Directory", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "The file extension for the output file(s)", ".csv"));
  params.insert(std::make_unique<Int32Parameter>(k_MaxValPerLine_Key, "Maximum Tuples Per Line", "Number of tuples to print on each line. Does not apply to string arrays", 1));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "The delimiter separating the data", to_underlying(OStreamUtilities::Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<ChoicesParameter>(k_Includes_Key, "Header and Index Options", "Default Include is Headers only", to_underlying(Includes::Headers),
                                                   ChoicesParameter::Choices{"Neither", "Headers", "Index", "Both"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "Data Arrays to be written to disk",
                                                               MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray, IArray::ArrayType::StringArray}, nx::core::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_MaxValPerLine_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_OutputDir_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));

  params.linkParameters(k_OutputStyle_Key, k_Includes_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));
  params.linkParameters(k_OutputStyle_Key, k_OutputPath_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));

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

  /////////////////////////////////////////////////////////////////////////////
  // VALIDATE THAT ALL DATA ARRAYS HAVE THE SAME NUMBER OF TUPLES.
  if(static_cast<WriteASCIIDataFilter::OutputStyle>(pOutputStyleValue) == WriteASCIIDataFilter::OutputStyle::SingleFile)
  {
    auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
    if(pSelectedDataArrayPathsValue.empty())
    {
      return MakePreflightErrorResult(k_NoArraySelections, "At least 1 data array must be selected");
    }

    if(!CheckArraysHaveSameTupleCount(dataStructure, pSelectedDataArrayPathsValue))
    {
      return MakePreflightErrorResult(k_UnmatchingTupleCountError, "Arrays do not all have the same length, a requirement for single file.");
    }
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteASCIIDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  const auto includes = static_cast<WriteASCIIDataFilter::Includes>(filterArgs.value<ChoicesParameter::ValueType>(k_Includes_Key));
  bool includeHeaders;
  bool includeIndex;
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
  auto selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto fileType = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);

  if(static_cast<WriteASCIIDataFilter::OutputStyle>(fileType) == WriteASCIIDataFilter::OutputStyle::SingleFile)
  {
    auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key));
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    auto outputPath = atomicFile.tempFilePath();
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(outputPath.parent_path());
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }

    // Scope file writer in code block to get around file lock on windows (enforce destructor order)
    {
      // Create the output file
      std::ofstream outStrm(outputPath, std::ios_base::out | std::ios_base::binary);
      if(!outStrm.is_open())
      {
        return MakeErrorResult(-11021, fmt::format("Unable to create output file {}", outputPath.string()));
      }

      OStreamUtilities::PrintDataSetsToSingleFile(outStrm, selectedDataArrayPaths, dataStructure, messageHandler, shouldCancel, delimiter, includeIndex, includeHeaders);
    }

    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  if(static_cast<WriteASCIIDataFilter::OutputStyle>(fileType) == WriteASCIIDataFilter::OutputStyle::MultipleFiles)
  {
    auto directoryPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDir_Key);
    auto fileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
    auto maxValPerLine = filterArgs.value<int32>(k_MaxValPerLine_Key);

    if(!fs::exists(directoryPath))
    {
      std::error_code err;
      if(!fs::create_directories(directoryPath, err))
      {
        return MakeErrorResult(-11022,
                               fmt::format("Unable to create output directory '{}'. Operating system returned error '{}' with message\n    '{}'", directoryPath.string(), err.value(), err.message()));
      }
    }
    return OStreamUtilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, dataStructure, directoryPath.string(), messageHandler, shouldCancel, fileExtension, false, delimiter, includeIndex,
                                                          includeHeaders, maxValPerLine);
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputStyleKey = "OutputStyle";
constexpr StringLiteral k_OutputPathKey = "OutputPath";
constexpr StringLiteral k_FileExtensionKey = "FileExtension";
constexpr StringLiteral k_MaxValPerLineKey = "MaxValPerLine";
constexpr StringLiteral k_OutputFilePathKey = "OutputFilePath";
constexpr StringLiteral k_DelimiterKey = "Delimiter";
constexpr StringLiteral k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteASCIIDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteASCIIDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_OutputStyleKey, k_OutputStyle_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputPathKey, k_OutputDir_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FileExtensionKey, k_FileExtension_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_MaxValPerLineKey, k_MaxValPerLine_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFilePathKey, k_OutputPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DelimiterKey, k_Delimiter_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
