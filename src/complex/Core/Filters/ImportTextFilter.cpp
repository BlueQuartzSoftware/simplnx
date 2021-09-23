#include "ImportTextFilter.hpp"

#include <filesystem>
#include <fstream>

#include "complex/Core/Parameters/ArrayCreationParameter.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/FileSystemPathParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/NumericTypeParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr const char k_InputFileKey[] = "input_file";
constexpr const char k_ScalarTypeKey[] = "scalar_type";
constexpr const char k_NTuplesKey[] = "n_tuples";
constexpr const char k_NCompKey[] = "n_comp";
constexpr const char k_NSkipLinesKey[] = "n_skip_lines";
constexpr const char k_DelimiterChoiceKey[] = "delimiter_choice";
constexpr const char k_DataArrayKey[] = "output_data_array";

class DelimiterType : public std::ctype<char>
{
  mask my_table[table_size];

public:
  DelimiterType(char delimiter, size_t refs = 0)
  : std::ctype<char>(&my_table[0], false, refs)
  {
    std::copy_n(classic_table(), table_size, my_table);
    my_table[static_cast<mask>(delimiter)] = (mask)space;
  }
};

template <class T>
Result<> ReadFile(const fs::path& inputPath, DataArray<T>& data, u64 skipLines, char delimiter)
{
  std::ifstream inputFile(inputPath, std::ios_base::binary);

  for(u64 i = 0; i < skipLines; i++)
  {
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  inputFile.imbue(std::locale(std::locale(), new DelimiterType(delimiter)));

  usize numTuples = data.getTupleCount();
  usize scalarNumComp = data.getNumComponents();

  usize totalSize = numTuples * scalarNumComp;
  T value = {};
  for(usize i = 0; i < totalSize; i++)
  {
    inputFile >> value;
    if(inputFile.fail())
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-1, "Unable to convert value"}})};
    }
    data[i] = value;
  }

  Result<> result;
  result.warnings().push_back(Warning{-1, "Test Warning"});

  return {};
}

template <class T>
DataArray<T>& ArrayFromPath(DataStructure& data, const DataPath& path)
{
  DataObject* object = data.getData(path);
  DataArray<T>* dataArray = dynamic_cast<DataArray<T>*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return *dataArray;
}

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
} // namespace

namespace complex
{
std::string ImportTextFilter::name() const
{
  return FilterTraits<ImportTextFilter>::name;
}

Uuid ImportTextFilter::uuid() const
{
  return FilterTraits<ImportTextFilter>::uuid;
}

std::string ImportTextFilter::humanName() const
{
  return "Import Text Data";
}

Parameters ImportTextFilter::parameters() const
{
  Parameters params;
  complex::FileSystemPathParameter::ValueType fspi("<default files to read goes here>");
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFileKey, "Input File", "File to read from", fspi, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarTypeKey, "Scalar Type", "Type to interpret data as", NumericType::i8));
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

Result<OutputActions> ImportTextFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto arrayPath = args.value<DataPath>(k_DataArrayKey);
  auto nTuples = args.value<u64>(k_NTuplesKey);
  auto nComp = args.value<u64>(k_NCompKey);
  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{nTuples}, nComp, arrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> ImportTextFilter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto inputFilePath = args.value<FileSystemPathParameter::ValueType>(k_InputFileKey);
  auto numericType = args.value<NumericType>(k_ScalarTypeKey);
  auto components = args.value<u64>(k_NCompKey);
  auto skipLines = args.value<u64>(k_NSkipLinesKey);
  auto choiceIndex = args.value<u64>(k_DelimiterChoiceKey);
  auto path = args.value<DataPath>(k_DataArrayKey);

  char delimiter = IndexToDelimiter(choiceIndex);

  switch(numericType)
  {
  case NumericType::i8: {
    auto& dataArray = ArrayFromPath<i8>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u8: {
    auto& dataArray = ArrayFromPath<u8>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i16: {
    auto& dataArray = ArrayFromPath<i16>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u16: {
    auto& dataArray = ArrayFromPath<u16>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i32: {
    auto& dataArray = ArrayFromPath<i32>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u32: {
    auto& dataArray = ArrayFromPath<u32>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::i64: {
    auto& dataArray = ArrayFromPath<i64>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::u64: {
    auto& dataArray = ArrayFromPath<u64>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::f32: {
    auto& dataArray = ArrayFromPath<f32>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  case NumericType::f64: {
    auto& dataArray = ArrayFromPath<f64>(data, path);
    return ReadFile(inputFilePath, dataArray, skipLines, delimiter);
  }
  default:
    throw std::runtime_error("Invalid type");
  }

  return {};
}
} // namespace complex
