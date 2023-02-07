#include "ImportTextFilter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/TypesUtility.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/Text/CsvParser.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace complex
{
std::string ImportTextFilter::name() const
{
  return FilterTraits<ImportTextFilter>::name;
}

std::string ImportTextFilter::className() const
{
  return FilterTraits<ImportTextFilter>::className;
}

Uuid ImportTextFilter::uuid() const
{
  return FilterTraits<ImportTextFilter>::uuid;
}

std::vector<std::string> ImportTextFilter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import", "Text"};
}

std::string ImportTextFilter::humanName() const
{
  return "Import ASCII Data Array";
}

Parameters ImportTextFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFileKey, "Input File", "File path that points to the imported file", fs::path("<file to import goes here>"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarTypeKey, "Scalar Type", "Data Type to interpret and store data into.", NumericType::int8));
  params.insert(std::make_unique<UInt64Parameter>(k_NTuplesKey, "Number of Tuples", "Number of tuples in resulting array (i.e. number of lines to read)", 0));
  params.insert(std::make_unique<UInt64Parameter>(k_NCompKey, "Number of Components", "Number of columns", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLinesKey, "Skip Header Lines", "Number of lines at the start of the file to skip", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceKey, "Delimiter", "Delimiter for values on a line", 0,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)"}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayKey, "Created Array Path", "DataPath or Name for the underlying Data Array", DataPath{}));
  return params;
}

IFilter::UniquePointer ImportTextFilter::clone() const
{
  return std::make_unique<ImportTextFilter>();
}

IFilter::PreflightResult ImportTextFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto arrayPath = args.value<DataPath>(k_DataArrayKey);
  auto nTuples = args.value<uint64>(k_NTuplesKey);
  auto nComp = args.value<uint64>(k_NCompKey);
  auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(numericType), std::vector<usize>{nTuples}, std::vector<usize>{nComp}, arrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> ImportTextFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto inputFilePath = args.value<fs::path>(k_InputFileKey);
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);

  // auto components = args.value<uint64>(k_NCompKey);
  auto skipLines = args.value<uint64>(k_NSkipLinesKey);
  auto choiceIndex = args.value<uint64>(k_DelimiterChoiceKey);
  auto path = args.value<DataPath>(k_DataArrayKey);

  char delimiter = complex::CsvParser::IndexToDelimiter(choiceIndex);

  switch(numericType)
  {
  case NumericType::int8: {
    auto dataArray = complex::ArrayFromPath<int8>(data, path);
    return CsvParser::ReadFile<int8_t, int32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint8: {
    auto dataArray = complex::ArrayFromPath<uint8>(data, path);
    return CsvParser::ReadFile<uint8_t, uint32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int16: {
    auto dataArray = complex::ArrayFromPath<int16>(data, path);
    return CsvParser::ReadFile<int16_t, int16_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint16: {
    auto dataArray = complex::ArrayFromPath<uint16>(data, path);
    return CsvParser::ReadFile<uint16_t, uint16_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int32: {
    auto dataArray = complex::ArrayFromPath<int32>(data, path);
    return CsvParser::ReadFile<int32_t, int32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint32: {
    auto dataArray = complex::ArrayFromPath<uint32>(data, path);
    return CsvParser::ReadFile<uint32_t, uint32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int64: {
    auto dataArray = complex::ArrayFromPath<int64>(data, path);
    return CsvParser::ReadFile<int64_t, int64_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint64: {
    auto dataArray = complex::ArrayFromPath<uint64>(data, path);
    return CsvParser::ReadFile<uint64_t, uint64_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::float32: {
    auto dataArray = complex::ArrayFromPath<float32>(data, path);
    return CsvParser::ReadFile<float, float>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::float64: {
    auto dataArray = complex::ArrayFromPath<float64>(data, path);
    return CsvParser::ReadFile<double, double>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  default:
    return MakeErrorResult(-1001, fmt::format("ImportTextFilter: Parameter NumericType which has a value of '{}' does not match any in complex.", to_underlying(numericType)));
  }
}
} // namespace complex
