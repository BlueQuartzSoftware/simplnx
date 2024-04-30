#include "ReadCSVFileFilter.hpp"

#include "SimplnxCore/utils/CSVDataParser.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <cstdio>
#include <fstream>

using namespace nx::core;

using ParsersVector = std::vector<std::unique_ptr<AbstractDataParser>>;
using StringVector = std::vector<std::string>;
using CharVector = std::vector<char>;
using DataTypeVector = std::vector<DataType>;
using Dimensions = std::vector<usize>;
namespace fs = std::filesystem;

namespace
{
struct ReadCSVFileFilterCache
{
  std::string FilePath;
  usize TotalLines = 0;
  usize HeadersLine = 0;
  std::string Headers;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ReadCSVFileFilterCache> s_HeaderCache;

enum class IssueCodes
{
  EMPTY_FILE = -100,
  EMPTY_NEW_DG = -102,
  EMPTY_EXISTING_DG = -103,
  INCONSISTENT_COLS = -104,
  DUPLICATE_NAMES = -105,
  INVALID_ARRAY_TYPE = -106,
  ILLEGAL_NAMES = -107,
  FILE_NOT_OPEN = -108,
  INCORRECT_DATATYPE_COUNT = -109,
  INCORRECT_MASK_COUNT = -110,
  INCORRECT_TUPLES = -113,
  NEW_DG_EXISTS = -114,
  CANNOT_SKIP_TO_LINE = -115,
  EMPTY_NAMES = -116,
  EMPTY_LINE = -119,
  HEADER_LINE_OUT_OF_RANGE = -120,
  START_IMPORT_ROW_OUT_OF_RANGE = -121,
  EMPTY_HEADERS = -122,
  IGNORED_TUPLE_DIMS = -200
};

// -----------------------------------------------------------------------------
Result<OutputActions> validateExistingGroup(const DataPath& groupPath, const DataStructure& dataStructure, const std::vector<std::string>& headers)
{
  if(groupPath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_EXISTING_DG), "'Existing Attribute Matrix' - Data path is empty.")};
  }

  const BaseGroup& selectedGroup = dataStructure.getDataRefAs<BaseGroup>(groupPath);
  const auto arrays = selectedGroup.findAllChildrenOfType<IDataArray>();
  for(const std::shared_ptr<IDataArray>& array : arrays)
  {
    std::string arrayName = array->getName();
    for(const std::string& headerName : headers)
    {
      if(arrayName == headerName)
      {
        return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::DUPLICATE_NAMES),
                                               fmt::format("The header name \"{}\" matches an array name that already exists in the selected container.", headerName))};
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<OutputActions> validateNewGroup(const DataPath& groupPath, const DataStructure& dataStructure)
{
  if(groupPath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_NEW_DG), "'New Data Group' - Data path is empty.")};
  }

  if(dataStructure.getData(groupPath) != nullptr)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::NEW_DG_EXISTS), fmt::format("The group at the path '{}' cannot be created because it already exists.", groupPath.toString()))};
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<ParsersVector> createParsers(const DataTypeVector& dataTypes, const std::vector<bool>& skippedArrays, const DataPath& parentPath, const std::vector<std::string>& headers,
                                    DataStructure& dataStructure)
{
  ParsersVector dataParsers(dataTypes.size());

  for(usize i = 0; i < dataTypes.size() && i < headers.size() && i < skippedArrays.size(); i++)
  {
    DataType dataType = dataTypes[i];
    std::string name = headers[i];
    bool skipped = skippedArrays[i];

    if(skipped)
    {
      continue;
    }

    DataPath arrayPath = parentPath;
    arrayPath = arrayPath.createChildPath(name);

    switch(dataType)
    {
    case nx::core::DataType::int8: {
      Int8Array& data = dataStructure.getDataRefAs<Int8Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int8Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::uint8: {
      UInt8Array& data = dataStructure.getDataRefAs<UInt8Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt8Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::int16: {
      Int16Array& data = dataStructure.getDataRefAs<Int16Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int16Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::uint16: {
      UInt16Array& data = dataStructure.getDataRefAs<UInt16Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt16Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::int32: {
      Int32Array& data = dataStructure.getDataRefAs<Int32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int32Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::uint32: {
      UInt32Array& data = dataStructure.getDataRefAs<UInt32Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt32Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::int64: {
      Int64Array& data = dataStructure.getDataRefAs<Int64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int64Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::uint64: {
      UInt64Array& data = dataStructure.getDataRefAs<UInt64Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt64Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::float32: {
      Float32Array& data = dataStructure.getDataRefAs<Float32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float32Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::float64: {
      Float64Array& data = dataStructure.getDataRefAs<Float64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float64Parser>(data, name, i);
      break;
    }
    case nx::core::DataType::boolean: {
      BoolArray& data = dataStructure.getDataRefAs<BoolArray>(arrayPath);
      dataParsers[i] = std::make_unique<BoolParser>(data, name, i);
      break;
    }
    default:
      return {MakeErrorResult<ParsersVector>(to_underlying(IssueCodes::INVALID_ARRAY_TYPE),
                                             fmt::format("The data type that was chosen for column number {} is not a valid data array type.", std::to_string(i + 1)))};
    }
  }

  return {std::move(dataParsers)};
}

// -----------------------------------------------------------------------------
Result<> parseLine(std::fstream& inStream, const ParsersVector& dataParsers, const StringVector& headers, const CharVector& delimiters, bool consecutiveDelimiters, usize lineNumber, usize beginIndex)
{
  std::string line;
  std::getline(inStream, line);

  StringVector tokens = StringUtilities::split(line, delimiters, consecutiveDelimiters);
  if(tokens.empty())
  {
    // This is an empty line in the middle of the CSV file, which just shouldn't happen
    return MakeErrorResult(to_underlying(IssueCodes::EMPTY_LINE), fmt::format("Line #{} is empty!  You should not have any empty lines in the file.", std::to_string(lineNumber)));
  }

  if(dataParsers.size() != tokens.size())
  {
    return MakeErrorResult(to_underlying(IssueCodes::INCONSISTENT_COLS),
                           fmt::format("Expecting {} tokens but found {} tokens in the file at line #{}.\n\nInput line was:\n{}\n\nThis is because the data-"
                                       "types/headers/skipped-array-mask all have a size of {} but the file data at line #{} has a column count of {}.",
                                       std::to_string(dataParsers.size()), std::to_string(tokens.size()), std::to_string(lineNumber), line, std::to_string(dataParsers.size()),
                                       std::to_string(lineNumber), std::to_string(tokens.size())));
  }

  for(int i = 0; i < dataParsers.size(); i++)
  {
    const auto& dataParser = dataParsers[i];
    if(dataParser == nullptr)
    {
      continue;
    }

    usize index = dataParser->columnIndex();

    Result<> result = dataParser->parse(tokens[index], lineNumber - beginIndex);
    if(result.invalid())
    {
      for(Error& error : result.errors())
      {
        error.message = fmt::format("Array \"{}\", Line {}: ", headers[i], lineNumber) + error.message;
      }
      return result;
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
void notifyProgress(const IFilter::MessageHandler& messageHandler, usize lineNumber, usize numberOfTuples, float32& threshold)
{
  const float32 percentCompleted = (static_cast<float32>(lineNumber) / static_cast<float32>(numberOfTuples)) * 100.0f;
  if(percentCompleted > threshold)
  {
    // Print the status of the import
    messageHandler({IFilter::Message::Type::Info, fmt::format("Importing CSV Data || {:.{}f}% Complete", static_cast<double>(percentCompleted), 1)});
    threshold = threshold + 5.0f;
    if(threshold < percentCompleted)
    {
      threshold = percentCompleted;
    }
  }
}

// -----------------------------------------------------------------------------
bool skipNumberOfLines(std::fstream& inStream, usize numberOfLines)
{
  for(usize i = 1; i < numberOfLines; i++)
  {
    if(inStream.eof())
    {
      return false;
    }

    std::string line;
    std::getline(inStream, line);
  }

  return true;
}

std::string tupleDimsToString(const std::vector<usize>& tupleDims)
{
  std::string tupleDimsStr;
  for(usize i = 0; i < tupleDims.size(); ++i)
  {
    tupleDimsStr += std::to_string(tupleDims[i]);
    if(i != tupleDims.size() - 1)
    {
      tupleDimsStr += "x";
    }
  }
  return tupleDimsStr;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult readHeaders(const std::string& inputFilePath, usize headersLineNum, ReadCSVFileFilterCache& headerCache)
{
  std::fstream in(inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath)), {}};
  }

  // Skip to the headers line
  if(!skipNumberOfLines(in, headersLineNum))
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::CANNOT_SKIP_TO_LINE), fmt::format("Could not skip to the chosen header line ({}).", headersLineNum)), {}};
  }

  // Read the headers line
  std::getline(in, headerCache.Headers);
  headerCache.HeadersLine = headersLineNum;
  return {};
}

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadCSVFileFilter::ReadCSVFileFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

// -----------------------------------------------------------------------------
ReadCSVFileFilter::~ReadCSVFileFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

// -----------------------------------------------------------------------------
std::string ReadCSVFileFilter::name() const
{
  return FilterTraits<ReadCSVFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadCSVFileFilter::className() const
{
  return FilterTraits<ReadCSVFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadCSVFileFilter::uuid() const
{
  return FilterTraits<ReadCSVFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadCSVFileFilter::humanName() const
{
  return "Read CSV File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadCSVFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "ASCII", "CSV", "Column", "Delimited", "Text"};
}

//------------------------------------------------------------------------------
Parameters ReadCSVFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<ReadCSVFileParameter>(k_ReadCSVData_Key, "CSV Importer Data", "Holds all relevant csv file data collected from the custom interface", ReadCSVData()));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "Value {}"));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Dim 0"}));

  params.insertSeparator(Parameters::Separator{"Input Attribute Matrix"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseExistingGroup_Key, "Use Existing Attribute Matrix", "Store the imported CSV data arrays in an existing attribute matrix.", false));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Existing Attribute Matrix",
                                                                    "Store the imported CSV data arrays in this existing attribute matrix.", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created AttributeMatrix"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDataGroup_Key, "New Attribute Matrix", "Store the imported CSV data arrays in a newly created attribute matrix.",
                                                             DataPath{{"Imported Data"}}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseExistingGroup_Key, k_SelectedAttributeMatrixPath_Key, true);
  params.linkParameters(k_UseExistingGroup_Key, k_CreatedDataGroup_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadCSVFileFilter::clone() const
{
  return std::make_unique<ReadCSVFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadCSVFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  ReadCSVData readCSVData = filterArgs.value<ReadCSVData>(k_ReadCSVData_Key);
  bool useExistingAM = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedAM = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  DataPath createdDataAM = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = readCSVData.inputFilePath;
  ReadCSVData::HeaderMode headerMode = readCSVData.headerMode;

  nx::core::Result<OutputActions> resultOutputActions;

  // Validate the input file path
  if(inputFilePath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_FILE), "A file has not been chosen to import. Please pick a file to import.")};
  }

  Result<> csvResult = FileUtilities::ValidateCSVFile(inputFilePath);
  if(csvResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(csvResult))), {}), {}};
  }

  StringVector headers;
  if(readCSVData.inputFilePath != s_HeaderCache[s_InstanceId].FilePath)
  {
    int64 lineCount = nx::core::FileUtilities::LinesInFile(inputFilePath);
    if(lineCount < 0)
    {
      return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath)), {}};
    }
    std::fstream in(inputFilePath.c_str(), std::ios_base::in);
    if(!in.is_open())
    {
      return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath)), {}};
    }

    s_HeaderCache[s_InstanceId].FilePath = readCSVData.inputFilePath;

    usize currentLine = 0;
    while(!in.eof())
    {
      std::string line;
      std::getline(in, line);
      currentLine++;

      if(headerMode == ReadCSVData::HeaderMode::LINE && currentLine == readCSVData.headersLine)
      {
        s_HeaderCache[s_InstanceId].Headers = line;
        s_HeaderCache[s_InstanceId].HeadersLine = readCSVData.headersLine;
        break;
      }
    }

    headers = StringUtilities::split(s_HeaderCache[s_InstanceId].Headers, readCSVData.delimiters, readCSVData.consecutiveDelimiters);
    s_HeaderCache[s_InstanceId].TotalLines = lineCount;
  }
  else if(headerMode == ReadCSVData::HeaderMode::LINE)
  {
    if(readCSVData.headersLine != s_HeaderCache[s_InstanceId].HeadersLine)
    {
      IFilter::PreflightResult result = readHeaders(readCSVData.inputFilePath, readCSVData.headersLine, s_HeaderCache[s_InstanceId]);
      if(result.outputActions.invalid())
      {
        return result;
      }
    }

    headers = StringUtilities::split(s_HeaderCache[s_InstanceId].Headers, readCSVData.delimiters, readCSVData.consecutiveDelimiters);
  }

  if(headerMode == ReadCSVData::HeaderMode::CUSTOM)
  {
    headers = readCSVData.customHeaders;
  }

  usize totalLines = s_HeaderCache[s_InstanceId].TotalLines;

  // Check that we have a valid start import row
  if(readCSVData.startImportRow == 0)
  {
    std::string errMsg = "'Start import at row' value is out of range.  The 'Start import at row' value cannot be set to line #0.";
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::START_IMPORT_ROW_OUT_OF_RANGE), errMsg), {}};
  }

  if(readCSVData.startImportRow > totalLines)
  {
    std::string errMsg = fmt::format("'Start import at row' value ({}) is larger than the total number of lines in the file ({}).", readCSVData.startImportRow, totalLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::START_IMPORT_ROW_OUT_OF_RANGE), errMsg), {}};
  }

  // Check that we have a valid header line number
  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine == 0)
  {
    std::string errMsg = "The header line number is out of range.  The header line number cannot be set to line #0.";
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine > totalLines)
  {
    std::string errMsg = fmt::format("The header line number is out of range.  There are {} lines in the file and the header line number is set to line #{}.", totalLines, readCSVData.headersLine);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine > readCSVData.startImportRow)
  {
    std::string errMsg = fmt::format(
        "The header line number is out of range.  The start import row is set to line #{} and the header line number is set to line #{}.  The header line number must be in the range 1-{}.",
        readCSVData.startImportRow, readCSVData.headersLine, readCSVData.startImportRow - 1);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  // Check that we have valid headers
  if(headers.empty())
  {
    std::string errMsg = "There are 0 imported array headers.  This is either because there are 0 headers being read from the header line or the custom headers are empty.  Please either choose a "
                         "different header line number or input at least 1 custom header.";
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_HEADERS), errMsg), {}};
  }

  if(readCSVData.dataTypes.size() != headers.size())
  {
    std::string errMsg =
        fmt::format("The number of data types ({}) does not match the number of imported array headers ({}).  The number of data types must match the number of imported array headers.",
                    readCSVData.dataTypes.size(), headers.size());
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_DATATYPE_COUNT), errMsg), {}};
  }

  if(readCSVData.skippedArrayMask.size() != headers.size())
  {
    std::string errMsg = fmt::format(
        "The number of booleans in the skipped array mask ({}) does not match the number of imported array headers ({}).  The number of booleans in the skipped array mask must match the number "
        "of imported array headers.",
        readCSVData.skippedArrayMask.size(), headers.size());
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_MASK_COUNT), errMsg), {}};
  }

  for(int i = 0; i < headers.size(); i++)
  {
    const auto& headerName = headers[i];
    if(headerName.empty())
    {
      std::string errMsg = fmt::format("The header for column #{} is empty.  Please fill in a header for column #{}.", i + 1, i + 1);
      return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_NAMES), errMsg), {}};
    }

    if(StringUtilities::contains(headerName, '&') || StringUtilities::contains(headerName, ':') || StringUtilities::contains(headerName, '/') || StringUtilities::contains(headerName, '\\'))
    {
      return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::ILLEGAL_NAMES),
                                             fmt::format("The header name \"{}\" contains a character that will cause problems. Do Not use '&',':', '/' or '\\' in the header names.", headerName))};
    }

    for(int j = 0; j < headers.size(); j++)
    {
      std::string otherHeaderName = headers[j];

      if(i != j && !headerName.empty() && !otherHeaderName.empty() && headerName == otherHeaderName)
      {
        std::string errMsg = fmt::format("Header '{}' (column #{}) and header '{}' (column #{}) have the same name.  Headers may not have duplicate names.", headerName, i + 1, otherHeaderName, j + 1);
        return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::DUPLICATE_NAMES), errMsg), {}};
      }
    }
  }

  // Check that we have a valid tuple count
  usize totalImportedLines = totalLines - readCSVData.startImportRow + 1;
  usize tupleTotal = std::accumulate(readCSVData.tupleDims.begin(), readCSVData.tupleDims.end(), static_cast<usize>(1), std::multiplies<usize>());
  if(tupleTotal == 0)
  {
    std::string tupleDimsStr = tupleDimsToString(readCSVData.tupleDims);
    std::string errMsg = fmt::format("Error: The current tuple dimensions ({}) has 0 total tuples.  At least 1 tuple is required.", tupleDimsStr, tupleTotal, totalImportedLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_TUPLES), errMsg), {}};
  }
  else if(tupleTotal > totalImportedLines && !useExistingAM)
  {
    std::string tupleDimsStr = tupleDimsToString(readCSVData.tupleDims);
    std::string errMsg = fmt::format("Error: The current tuple dimensions ({}) has {} total tuples, but this is larger than the total number of available lines to import ({}).", tupleDimsStr,
                                     tupleTotal, totalImportedLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_TUPLES), errMsg), {}};
  }

  // Validate the existing/created group
  DataPath groupPath;
  if(useExistingAM)
  {
    Result<OutputActions> result = validateExistingGroup(selectedAM, dataStructure, headers);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = selectedAM;
  }
  else
  {
    Result<OutputActions> result = validateNewGroup(createdDataAM, dataStructure);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = createdDataAM;
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(createdDataAM, readCSVData.tupleDims));
  }

  // Create the arrays
  std::vector<usize> tupleDims(readCSVData.tupleDims.size());
  std::transform(readCSVData.tupleDims.begin(), readCSVData.tupleDims.end(), tupleDims.begin(), [](usize d) { return d; });
  if(useExistingAM)
  {
    const AttributeMatrix& am = dataStructure.getDataRefAs<AttributeMatrix>(groupPath);
    tupleDims = am.getShape();

    auto totalLinesRead = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<usize>(1), std::multiplies<>());

    std::string msg = fmt::format("The Array Tuple Dimensions ({}) will be ignored and the Existing Attribute Matrix tuple dimensions ({}) will be used. The total number of lines read will be {}.",
                                  fmt::join(readCSVData.tupleDims, "x"), fmt::join(tupleDims, "x"), totalLinesRead);
    resultOutputActions.warnings().push_back(Warning{to_underlying(IssueCodes::IGNORED_TUPLE_DIMS), msg});
  }

  for(usize i = 0; i < headers.size(); i++)
  {
    if(readCSVData.skippedArrayMask[i])
    {
      // The user decided to skip importing this array
      continue;
    }

    DataType dataType = readCSVData.dataTypes[i];
    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath));
  }

  return {std::move(resultOutputActions), {}};
}

//------------------------------------------------------------------------------
Result<> ReadCSVFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ReadCSVData readCSVData = filterArgs.value<ReadCSVData>(k_ReadCSVData_Key);
  bool useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedDataGroup = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  DataPath createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = readCSVData.inputFilePath;
  StringVector headers = StringUtilities::split(s_HeaderCache[s_InstanceId].Headers, readCSVData.delimiters, readCSVData.consecutiveDelimiters);
  DataTypeVector dataTypes = readCSVData.dataTypes;
  std::vector<bool> skippedArrays = readCSVData.skippedArrayMask;
  bool consecutiveDelimiters = readCSVData.consecutiveDelimiters;
  usize startImportRow = readCSVData.startImportRow;

  if(readCSVData.headerMode == ReadCSVData::HeaderMode::CUSTOM)
  {
    headers = readCSVData.customHeaders;
  }

  DataPath groupPath = createdDataGroup;
  if(useExistingGroup)
  {
    groupPath = selectedDataGroup;
  }

  Result<ParsersVector> parsersResult = createParsers(dataTypes, skippedArrays, groupPath, headers, dataStructure);
  if(parsersResult.invalid())
  {
    return ConvertResult(std::move(parsersResult));
  }

  std::fstream in(inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  // Skip to the first data line
  if(!skipNumberOfLines(in, startImportRow))
  {
    return MakeErrorResult(to_underlying(IssueCodes::CANNOT_SKIP_TO_LINE), fmt::format("Could not skip to the first line in the file to import ({}).", startImportRow));
  }

  float32 threshold = 0.0f;
  usize numTuples = std::accumulate(readCSVData.tupleDims.cbegin(), readCSVData.tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());
  if(useExistingGroup)
  {
    const AttributeMatrix& am = dataStructure.getDataRefAs<AttributeMatrix>(groupPath);
    numTuples = std::accumulate(am.getShape().cbegin(), am.getShape().cend(), static_cast<usize>(1), std::multiplies<>());
  }
  usize lineNum = startImportRow;
  for(usize i = 0; i < numTuples && !in.eof(); i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    Result<> parsingResult = parseLine(in, parsersResult.value(), headers, readCSVData.delimiters, consecutiveDelimiters, lineNum, startImportRow);
    if(parsingResult.invalid())
    {
      return std::move(parsingResult);
    }

    notifyProgress(messageHandler, lineNum, numTuples, threshold);
    lineNum++;
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedPathKey = "Wizard_SelectedPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadCSVFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadCSVFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Wizard Data parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedPathKey, k_CreatedDataGroup_Key));
  // Tuple Dims parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
