#include "ImportTextFilter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/InputFileParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/NumericTypeParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr const char k_InputFileKey[] = "input_file";
constexpr const char k_ScalarTypeKey[] = "scalar_type";
constexpr const char k_NCompKey[] = "n_comp";
constexpr const char k_NSkipLinesKey[] = "n_skip_lines";
constexpr const char k_DelimiterChoiceKey[] = "delimiter_choice";
constexpr const char k_DataArrayKey[] = "output_data_array";

char IndexToDelimiter(u64 index)
{
  switch(index)
  {
  case 0:
    return ',';
  case 1:
    return ';';
  case 2:
    return ' ';
  case 3:
    return ':';
  case 4:
    return '\t';
  default:
    throw std::runtime_error("Invalid index");
  }
}

template <class T>
Result<> ReadFile(const fs::path& inputPath, DataArray<T>& data, u64 skipLines, char delimiter)
{
  std::ifstream inputFile(inputPath);
  std::string line;

  for(u64 i = 0; i < skipLines; i++)
  {
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), delimiter);
  }

  usize index = 0;
  while(std::getline(inputFile, line))
  {
    std::stringstream stream(line);
    while(stream >> data[index])
    {
      index++;
    }
  }

  return {};
}
} // namespace

namespace complex
{
std::string ImportTextFilter::name() const
{
  return "ImportTextFilter";
}

Uuid ImportTextFilter::uuid() const
{
  constexpr Uuid uuid = *Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4");
  return uuid;
}

std::string ImportTextFilter::humanName() const
{
  return "Import Text Data";
}

Parameters ImportTextFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<InputFileParameter>(k_InputFileKey, "Input File", "File to read from", ""));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarTypeKey, "Scalar Type", "Type to interpret data as", NumericType::i8));
  params.insert(std::make_unique<UInt64Parameter>(k_NCompKey, "Number of Components", "Number of columns", 0));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLinesKey, "Skip Header Lines", "Number of lines to skip in the file", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceKey, "Delimiter", "Delimiter for values on a line", 0,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)"}));
  return params;
}

Result<OutputActions> ImportTextFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> ImportTextFilter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto inputFilePath = args.value<fs::path>(k_InputFileKey);
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto components = args.value<u64>(k_NCompKey);
  auto skipLines = args.value<u64>(k_NSkipLinesKey);
  auto choiceIndex = args.value<u64>(k_DelimiterChoiceKey);

  char delimiter = IndexToDelimiter(choiceIndex);

  switch(numericType)
  {
  case NumericType::i8: {
    auto& dataArray = args.ref<DataArray<i8>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u8: {
    auto& dataArray = args.ref<DataArray<u8>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i16: {
    auto& dataArray = args.ref<DataArray<i16>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u16: {
    auto& dataArray = args.ref<DataArray<u16>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i32: {
    auto& dataArray = args.ref<DataArray<i32>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u32: {
    auto& dataArray = args.ref<DataArray<u32>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i64: {
    auto& dataArray = args.ref<DataArray<i64>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u64: {
    auto& dataArray = args.ref<DataArray<u64>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::f32: {
    auto& dataArray = args.ref<DataArray<f32>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::f64: {
    auto& dataArray = args.ref<DataArray<f64>>(k_DataArrayKey);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  default:
    throw std::runtime_error("Invalid type");
  }

  return {};
}
} // namespace complex
