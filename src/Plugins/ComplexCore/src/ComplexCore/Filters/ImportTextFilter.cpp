#include "ImportTextFilter.hpp"

#include <filesystem>

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Utilities/Parsing/Text/CsvParser.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr StringLiteral k_InputFileKey = "input_file";
constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
constexpr StringLiteral k_NTuplesKey = "n_tuples";
constexpr StringLiteral k_NCompKey = "n_comp";
constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
constexpr StringLiteral k_DataArrayKey = "output_data_array";

} // namespace

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

std::string ImportTextFilter::humanName() const
{
  return "Import ASCII Data";
}

Parameters ImportTextFilter::parameters() const
{
  Parameters params;
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputFileKey, "Input File", "File to read from", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarTypeKey, "Scalar Type", "Type to interpret data as", NumericType::int8));
  params.insert(std::make_unique<UInt64Parameter>(k_NTuplesKey, "Number of Tuples", "Number of tuples in resulting array (i.e. number of lines to read)", 3));
  params.insert(std::make_unique<UInt64Parameter>(k_NCompKey, "Number of Components", "Number of columns", 3));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLinesKey, "Skip Header Lines", "Number of lines to skip in the file", 7));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceKey, "Delimiter", "Delimiter for values on a line", 0,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)"}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayKey, "Created Array", "Array storing the file data", DataPath{}));
  return params;
}

IFilter::UniquePointer ImportTextFilter::clone() const
{
  return std::make_unique<ImportTextFilter>();
}

IFilter::PreflightResult ImportTextFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto arrayPath = args.value<DataPath>(k_DataArrayKey);
  auto nTuples = args.value<uint64>(k_NTuplesKey);
  auto nComp = args.value<uint64>(k_NCompKey);
  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{nTuples}, nComp, arrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> ImportTextFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
    auto& dataArray = complex::CsvParser::ArrayFromPath<int8>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::uint8: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<uint8>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::int16: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<int16>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::uint16: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<uint16>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::int32: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<int32>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::uint32: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<uint32>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::int64: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<int64>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::uint64: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<uint64>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::float32: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<float32>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::float64: {
    auto& dataArray = complex::CsvParser::ArrayFromPath<float64>(data, path);
    return CsvParser::ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  default:
    throw std::runtime_error("Invalid type");
  }
}
} // namespace complex
