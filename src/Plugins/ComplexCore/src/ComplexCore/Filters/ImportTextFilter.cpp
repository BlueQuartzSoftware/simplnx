#include "ImportTextFilter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/TypesUtility.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataStoreFormatParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
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
  params.insert(std::make_unique<UInt64Parameter>(k_NCompKey, "Number of Components", "Number of columns", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLinesKey, "Skip Header Lines", "Number of lines at the start of the file to skip", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceKey, "Delimiter", "Delimiter for values on a line", 0,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)"}));

  params.insertSeparator(Parameters::Separator{"Created DataArray"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayKey, "Created Array Path", "DataPath or Name for the underlying Data Array", DataPath{}));
  params.insert(std::make_unique<DataStoreFormatParameter>(k_DataFormat_Key, "Data Format",
                                                           "This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.", ""));

  params.insertSeparator(Parameters::Separator{"Tuple Handling"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them \n\nThis option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));
  params.insert(std::make_unique<DynamicTableParameter>(k_NTuplesKey, "Data Array Dimensions (Slowest to Fastest Dimensions)",
                                                        "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));

  params.linkParameters(k_AdvancedOptions_Key, k_NTuplesKey, true);

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
  auto nComp = args.value<uint64>(k_NCompKey);

  auto useDims = args.value<bool>(k_AdvancedOptions_Key);
  auto tableData = args.value<DynamicTableParameter::ValueType>(k_NTuplesKey);
  auto dataFormat = args.value<std::string>(k_DataFormat_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<usize> tupleDims = {};

  auto* parentAM = data.getDataAs<AttributeMatrix>(arrayPath.getParent());
  if(parentAM == nullptr)
  {
    if(!useDims)
    {
      return MakePreflightErrorResult(
          -77602, "The DataArray to be created is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. Check Set Tuple Dimensions to set the dimensions");
    }
    else
    {
      const auto& rowData = tableData.at(0);
      tupleDims.reserve(rowData.size());
      for(auto floatValue : rowData)
      {
        if(floatValue == 0)
        {
          return MakePreflightErrorResult(-77603, "Tuple dimension cannot be zero");
        }

        tupleDims.push_back(static_cast<usize>(floatValue));
      }
    }
  }
  else
  {
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back(
          Warning{-77604, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. The Attribute Matrix tuples will override your "
                          "custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }

  auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(numericType), tupleDims, std::vector<usize>{nComp}, arrayPath, dataFormat);

  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions)};
}

Result<> ImportTextFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto inputFilePath = args.value<fs::path>(k_InputFileKey);
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto skipLines = args.value<uint64>(k_NSkipLinesKey);
  auto choiceIndex = args.value<uint64>(k_DelimiterChoiceKey);
  auto path = args.value<DataPath>(k_DataArrayKey);

  char delimiter = complex::CsvParser::IndexToDelimiter(choiceIndex);

  switch(numericType)
  {
  case NumericType::int8: {
    auto dataArray = complex::ArrayFromPath<int8>(data, path);
    return CsvParser::ReadFile<int8_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint8: {
    auto dataArray = complex::ArrayFromPath<uint8>(data, path);
    return CsvParser::ReadFile<uint8_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int16: {
    auto dataArray = complex::ArrayFromPath<int16>(data, path);
    return CsvParser::ReadFile<int16_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint16: {
    auto dataArray = complex::ArrayFromPath<uint16>(data, path);
    return CsvParser::ReadFile<uint16_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int32: {
    auto dataArray = complex::ArrayFromPath<int32>(data, path);
    return CsvParser::ReadFile<int32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint32: {
    auto dataArray = complex::ArrayFromPath<uint32>(data, path);
    return CsvParser::ReadFile<uint32_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::int64: {
    auto dataArray = complex::ArrayFromPath<int64>(data, path);
    return CsvParser::ReadFile<int64_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::uint64: {
    auto dataArray = complex::ArrayFromPath<uint64>(data, path);
    return CsvParser::ReadFile<uint64_t>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::float32: {
    auto dataArray = complex::ArrayFromPath<float32>(data, path);
    return CsvParser::ReadFile<float>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  case NumericType::float64: {
    auto dataArray = complex::ArrayFromPath<float64>(data, path);
    return CsvParser::ReadFile<double>(inputFilePath, *dataArray, skipLines, delimiter);
  }
  default:
    return MakeErrorResult(-1001, fmt::format("ImportTextFilter: Parameter NumericType which has a value of '{}' does not match any in complex.", to_underlying(numericType)));
  }
}
} // namespace complex
