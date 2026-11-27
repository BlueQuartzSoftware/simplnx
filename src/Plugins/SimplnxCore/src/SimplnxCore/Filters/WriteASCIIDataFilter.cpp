#include "WriteASCIIDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteASCIIData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>
#include <map>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// Error Code constants
constexpr nx::core::int32 k_UnmatchingTupleCountError = -51001;
constexpr nx::core::int32 k_NoArraySelections = -51002;
constexpr nx::core::int32 k_DuplicateOutputName = -51003;
constexpr nx::core::int32 k_NegativeMaxTuplesPerLine = -51004;
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
                                                                    ChoicesParameter::ChoicesType{"Multiple Files", "Single File"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile, true));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputDir_Key, "Output Directory", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "The file extension for the output file(s)", ".csv"));
  params.insert(std::make_unique<Int32ParameterType>(k_MaxTuplePerLine_Key, "Maximum Tuples Per Line", "Number of tuples to print on each line. Does not apply to string arrays", 1));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "The delimiter separating the data", to_underlying(o_stream_utilities::Delimiter::Comma),
                                                   ChoicesParameter::ChoicesType{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<ChoicesParameter>(k_Includes_Key, "Header and Index Options", "Default Include is Headers only", to_underlying(Includes::Headers),
                                                   ChoicesParameter::ChoicesType{"Neither", "Headers", "Index", "Both"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "Data Arrays to be written to disk", MultiArraySelectionParameter::ValueType{},
                                                     MultiArraySelectionParameter::AllowedTypeSetType{IArray::ArrayType::DataArray, IArray::ArrayType::StringArray}, nx::core::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputStyle_Key, k_MaxTuplePerLine_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_OutputDir_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));
  params.linkParameters(k_OutputStyle_Key, k_FileExtension_Key, std::make_any<uint64>(to_underlying(OutputStyle::MultipleFiles)));

  params.linkParameters(k_OutputStyle_Key, k_Includes_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));
  params.linkParameters(k_OutputStyle_Key, k_OutputPath_Key, std::make_any<uint64>(to_underlying(OutputStyle::SingleFile)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteASCIIDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteASCIIDataFilter::clone() const
{
  return std::make_unique<WriteASCIIDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteASCIIDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                             const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  const auto selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  if(selectedDataArrayPaths.empty())
  {
    return MakePreflightErrorResult(k_NoArraySelections, "Select at least one DataArray in **Attribute Arrays to Export**.");
  }

  // A single output file requires the same number of tuples in each DataArray.
  if(static_cast<WriteASCIIDataFilter::OutputStyle>(pOutputStyleValue) == WriteASCIIDataFilter::OutputStyle::SingleFile)
  {
    if(!CheckArraysHaveSameTupleCount(dataStructure, selectedDataArrayPaths))
    {
      return MakePreflightErrorResult(k_UnmatchingTupleCountError, "Arrays do not all have the same length, a requirement for single file.");
    }
  }
  else if(static_cast<WriteASCIIDataFilter::OutputStyle>(pOutputStyleValue) == WriteASCIIDataFilter::OutputStyle::MultipleFiles)
  {
    const auto maxTuplesPerLine = filterArgs.value<Int32ParameterType::ValueType>(k_MaxTuplePerLine_Key);
    if(maxTuplesPerLine < 0)
    {
      return MakePreflightErrorResult(k_NegativeMaxTuplesPerLine, fmt::format("**Maximum Tuples Per Line** is {}. Enter a nonnegative value; 0 writes one tuple per line.", maxTuplesPerLine));
    }

    std::map<std::string, DataPath> outputNames;
    for(const auto& arrayPath : selectedDataArrayPaths)
    {
      // Some file systems treat names that differ only by case as the same file.
      const auto [iter, inserted] = outputNames.emplace(string_utilities::ToLower(arrayPath.getTargetName()), arrayPath);
      if(!inserted)
      {
        return MakePreflightErrorResult(k_DuplicateOutputName,
                                        fmt::format("In Multiple Files mode, DataArrays '{}' and '{}' have the same output name when case is ignored. Select DataArrays with different names.",
                                                    iter->second.toString(), arrayPath.toString()));
      }
    }
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteASCIIDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteASCIIDataInputValues inputValues;
  inputValues.output_style_index = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  inputValues.output_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.output_dir = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDir_Key);
  inputValues.file_extension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  inputValues.max_val_per_line = filterArgs.value<Int32ParameterType::ValueType>(k_MaxTuplePerLine_Key);
  inputValues.delimiter_index = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.header_option_index = filterArgs.value<ChoicesParameter::ValueType>(k_Includes_Key);
  inputValues.input_data_array_paths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  return WriteASCIIData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputStyleKey = "OutputStyle";
constexpr StringLiteralType k_OutputPathKey = "OutputPath";
constexpr StringLiteralType k_FileExtensionKey = "FileExtension";
constexpr StringLiteralType k_MaxValPerLineKey = "MaxValPerLine";
constexpr StringLiteralType k_OutputFilePathKey = "OutputFilePath";
constexpr StringLiteralType k_DelimiterKey = "Delimiter";
constexpr StringLiteralType k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
} // namespace simpl
} // namespace

Result<Arguments> WriteASCIIDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteASCIIDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedChoicesFilterParameterConverter>(args, json, simpl::k_OutputStyleKey, k_OutputStyle_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputPathKey, k_OutputDir_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_FileExtensionKey, k_FileExtension_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_MaxValPerLineKey, k_MaxTuplePerLine_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFilePathKey, k_OutputPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DelimiterKey, k_Delimiter_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
