#include "ImportCSVDataFilter.hpp"

#include "ComplexCore/utils/CSVDataParser.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/ImportCSVDataParameter.hpp"
#include "complex/Utilities/FileUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fstream>

using namespace complex;

using ParsersVector = std::vector<std::unique_ptr<AbstractDataParser>>;
using StringVector = std::vector<std::string>;
using CharVector = std::vector<char>;
using DataTypeVector = std::vector<DataType>;
using Dimensions = std::vector<usize>;
namespace fs = std::filesystem;

namespace
{
struct ImportCSVDataFilterCache
{
  std::string FilePath;
  usize TotalLines = 0;
  usize HeadersLine = 0;
  std::vector<std::string> Headers;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ImportCSVDataFilterCache> s_HeaderCache;

enum class IssueCodes
{
  EMPTY_FILE = -100,
  FILE_DOES_NOT_EXIST = -101,
  EMPTY_NEW_DG = -102,
  EMPTY_EXISTING_DG = -103,
  INCONSISTENT_COLS = -104,
  DUPLICATE_NAMES = -105,
  INVALID_ARRAY_TYPE = -106,
  ILLEGAL_NAMES = -107,
  FILE_NOT_OPEN = -108,
  MEMORY_ALLOCATION_FAIL = -109,
  UNABLE_TO_READ_DATA = -110,
  UNPRINTABLE_CHARACTERS = -111,
  BINARY_DETECTED = -112,
  INCORRECT_TUPLES = -113,
  NEW_DG_EXISTS = -114,
  CANNOT_SKIP_TO_LINE = -115,
  EMPTY_NAMES = -116,
  START_LINE_LARGER_THAN_TOTAL = -117,
  HEADER_LINE_LARGER_THAN_TOTAL = -118
};

// -----------------------------------------------------------------------------
Result<OutputActions> validateInputFilePath(const std::string& inputFilePath)
{
  if(inputFilePath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_FILE), "A file has not been chosen to import. Please pick a file to import.")};
  }

  fs::path inputFile(inputFilePath);
  if(!fs::exists(inputFile))
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::FILE_DOES_NOT_EXIST), fmt::format("The input file does not exist: '{}'", inputFilePath))};
  }

  return {};
}

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
      if(StringUtilities::contains(headerName, '&') || StringUtilities::contains(headerName, ':') || StringUtilities::contains(headerName, '/') || StringUtilities::contains(headerName, '\\'))
      {
        return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::ILLEGAL_NAMES),
                                               fmt::format("The header name \"{}\" contains a character that will cause problems. Do Not use '&',':', '/' or '\\' in the header names.", headerName))};
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
Result<ParsersVector> createParsers(const DataTypeVector& dataTypes, const DataPath& parentPath, const std::vector<std::string>& headers, DataStructure& dataStructure)
{
  ParsersVector dataParsers(dataTypes.size());

  for(usize i = 0; i < dataTypes.size() && i < headers.size(); i++)
  {
    DataType dataType = dataTypes[i];
    std::string name = headers[i];

    DataPath arrayPath = parentPath;
    arrayPath = arrayPath.createChildPath(name);

    switch(dataType)
    {
    case complex::DataType::int8: {
      Int8Array& data = dataStructure.getDataRefAs<Int8Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int8Parser>(data, name, i);
      break;
    }
    case complex::DataType::uint8: {
      UInt8Array& data = dataStructure.getDataRefAs<UInt8Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt8Parser>(data, name, i);
      break;
    }
    case complex::DataType::int16: {
      Int16Array& data = dataStructure.getDataRefAs<Int16Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int16Parser>(data, name, i);
      break;
    }
    case complex::DataType::uint16: {
      UInt16Array& data = dataStructure.getDataRefAs<UInt16Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt16Parser>(data, name, i);
      break;
    }
    case complex::DataType::int32: {
      Int32Array& data = dataStructure.getDataRefAs<Int32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int32Parser>(data, name, i);
      break;
    }
    case complex::DataType::uint32: {
      UInt32Array& data = dataStructure.getDataRefAs<UInt32Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt32Parser>(data, name, i);
      break;
    }
    case complex::DataType::int64: {
      Int64Array& data = dataStructure.getDataRefAs<Int64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int64Parser>(data, name, i);
      break;
    }
    case complex::DataType::uint64: {
      UInt64Array& data = dataStructure.getDataRefAs<UInt64Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt64Parser>(data, name, i);
      break;
    }
    case complex::DataType::float32: {
      Float32Array& data = dataStructure.getDataRefAs<Float32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float32Parser>(data, name, i);
      break;
    }
    case complex::DataType::float64: {
      Float64Array& data = dataStructure.getDataRefAs<Float64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float64Parser>(data, name, i);
      break;
    }
      //    case complex::DataType::string:
      //    {
      //      StringArray& data = dataStructure.getDataRefAs<StringArray>(arrayPath);
      //      dataParsers[i] = std::make_unique<StringParser>(data, name, i);
      //      break;
      //    }
    default:
      return {MakeErrorResult<ParsersVector>(to_underlying(IssueCodes::INVALID_ARRAY_TYPE),
                                             fmt::format("The data type that was chosen for column number {} is not a valid data array type.", std::to_string(i + 1)))};
    }
  }

  return {std::move(dataParsers)};
}

// -----------------------------------------------------------------------------
Result<> parseLine(std::fstream& inStream, const ParsersVector& dataParsers, const CharVector& delimiters, bool consecutiveDelimiters, usize lineNumber, usize beginIndex)
{
  std::string line;
  std::getline(inStream, line);

  StringVector tokens = StringUtilities::split(line, delimiters, consecutiveDelimiters);

  if(dataParsers.size() != tokens.size())
  {
    return MakeErrorResult(to_underlying(IssueCodes::INCONSISTENT_COLS), fmt::format("Line {} has an inconsistent number of columns.\nExpecting {} but found {}\nInput line was:\n{}",
                                                                                     std::to_string(lineNumber), std::to_string(dataParsers.size()), std::to_string(tokens.size()), line));
  }

  for(const auto& dataParser : dataParsers)
  {
    if(dataParser == nullptr)
    {
      continue;
    }

    usize index = dataParser->columnIndex();

    Result<> result = dataParser->parse(tokens[index], lineNumber - beginIndex);
    if(result.invalid())
    {
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

// -----------------------------------------------------------------------------
std::vector<char> createDelimitersVector(bool tabAsDelimiter, bool semicolonAsDelimiter, bool commaAsDelimiter, bool spaceAsDelimiter)
{
  std::vector<char> delimiters;
  if(tabAsDelimiter)
  {
    delimiters.push_back('\t');
  }
  if(semicolonAsDelimiter)
  {
    delimiters.push_back(';');
  }
  if(commaAsDelimiter)
  {
    delimiters.push_back(',');
  }
  if(spaceAsDelimiter)
  {
    delimiters.push_back(' ');
  }

  return delimiters;
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ImportCSVDataFilter::ImportCSVDataFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

// -----------------------------------------------------------------------------
ImportCSVDataFilter::~ImportCSVDataFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

// -----------------------------------------------------------------------------
std::string ImportCSVDataFilter::name() const
{
  return FilterTraits<ImportCSVDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportCSVDataFilter::className() const
{
  return FilterTraits<ImportCSVDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportCSVDataFilter::uuid() const
{
  return FilterTraits<ImportCSVDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportCSVDataFilter::humanName() const
{
  return "Import CSV Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportCSVDataFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "ASCII", "ascii", "CSV", "csv", "Column"};
}

//------------------------------------------------------------------------------
Parameters ImportCSVDataFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<ImportCSVDataParameter>(k_WizardData_Key, "CSV Wizard Data", "Holds all relevant csv file data collected from the wizard", CSVWizardData()));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "Value {}"));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Dim 0"}));

  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseExistingGroup_Key, "Use Existing Attribute Matrix", "Store the imported CSV data arrays in an existing attribute matrix.", false));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedDataGroup_Key, "Existing Data Group", "Store the imported CSV data arrays in this existing attribute matrix.", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDataGroup_Key, "New Attribute Matrix", "Store the imported CSV data arrays in a newly created attribute matrix.",
                                                             DataPath{{"Imported Data"}}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseExistingGroup_Key, k_SelectedDataGroup_Key, true);
  params.linkParameters(k_UseExistingGroup_Key, k_CreatedDataGroup_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportCSVDataFilter::clone() const
{
  return std::make_unique<ImportCSVDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportCSVDataFilter::readHeaders(const std::string& inputFilePath, usize headersLineNum, bool useTab, bool useSemicolon, bool useSpace, bool useComma,
                                                          bool useConsecutive) const
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
  std::string headersLine;
  std::getline(in, headersLine);

  std::vector<char> delimiters = createDelimitersVector(useTab, useSemicolon, useComma, useSpace);
  s_HeaderCache[s_InstanceId].Headers = StringUtilities::split(headersLine, delimiters, useConsecutive);
  s_HeaderCache[s_InstanceId].HeadersLine = headersLineNum;
  return {};
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportCSVDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  CSVWizardData wizardData = filterArgs.value<CSVWizardData>(k_WizardData_Key);
  bool useExistingAM = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedAM = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  DataPath createdDataAM = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = wizardData.inputFilePath;
  CSVWizardData::HeaderMode headerMode = wizardData.headerMode;

  Result<> csvResult = FileUtilities::ValidateCSVFile(inputFilePath);
  if(csvResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(csvResult))), {}), {}};
  }

  if(wizardData.inputFilePath != s_HeaderCache[s_InstanceId].FilePath)
  {
    std::fstream in(inputFilePath.c_str(), std::ios_base::in);
    if(!in.is_open())
    {
      return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath)), {}};
    }

    s_HeaderCache[s_InstanceId].FilePath = wizardData.inputFilePath;

    usize lineCount = 0;
    while(!in.eof())
    {
      std::string line;
      std::getline(in, line);
      lineCount++;

      if(headerMode == CSVWizardData::HeaderMode::LINE && wizardData.headersLine != s_HeaderCache[s_InstanceId].HeadersLine)
      {
        std::vector<char> delimiters = createDelimitersVector(wizardData.tabAsDelimiter, wizardData.semicolonAsDelimiter, wizardData.commaAsDelimiter, wizardData.spaceAsDelimiter);
        s_HeaderCache[s_InstanceId].Headers = StringUtilities::split(line, delimiters, wizardData.consecutiveDelimiters);
        s_HeaderCache[s_InstanceId].HeadersLine = wizardData.headersLine;
      }
    }

    s_HeaderCache[s_InstanceId].TotalLines = lineCount;
  }
  else if(headerMode == CSVWizardData::HeaderMode::LINE && wizardData.headersLine != s_HeaderCache[s_InstanceId].HeadersLine)
  {
    IFilter::PreflightResult result = readHeaders(wizardData.inputFilePath, wizardData.headersLine, wizardData.tabAsDelimiter, wizardData.semicolonAsDelimiter, wizardData.spaceAsDelimiter,
                                                  wizardData.commaAsDelimiter, wizardData.consecutiveDelimiters);
    if(result.outputActions.invalid())
    {
      return result;
    }
  }

  StringVector headers = (headerMode == CSVWizardData::HeaderMode::LINE) ? s_HeaderCache[s_InstanceId].Headers : wizardData.customHeaders;
  Dimensions cDims = {1};
  complex::Result<OutputActions> resultOutputActions;

  // Validate the input file path
  Result<OutputActions> result = validateInputFilePath(inputFilePath);
  if(result.invalid())
  {
    return {std::move(result)};
  }

  size_t totalLines = s_HeaderCache[s_InstanceId].TotalLines;
  size_t totalImportedLines = totalLines - wizardData.startImportRow + 1;
  size_t tupleTotal = std::accumulate(wizardData.tupleDims.begin(), wizardData.tupleDims.end(), static_cast<size_t>(1), std::multiplies<size_t>());
  if(tupleTotal != totalImportedLines)
  {
    std::string tupleDimsStr;
    for(size_t i = 0; i < wizardData.tupleDims.size(); ++i)
    {
      tupleDimsStr += std::to_string(wizardData.tupleDims[i]);
      if(i != wizardData.tupleDims.size() - 1)
      {
        tupleDimsStr += "x";
      }
    }

    std::string errMsg =
        fmt::format("Error: The current tuple dimensions ({}) has {} tuples, but this does not match the total number of imported lines ({}).", tupleDimsStr, tupleTotal, totalImportedLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_TUPLES), errMsg), {}};
  }

  // Validate the existing/created group
  DataPath groupPath;
  if(useExistingAM)
  {
    result = validateExistingGroup(selectedAM, dataStructure, headers);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = selectedAM;
  }
  else
  {
    result = validateNewGroup(createdDataAM, dataStructure);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = createdDataAM;
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(createdDataAM, wizardData.tupleDims));
  }

  if(wizardData.startImportRow > totalLines)
  {
    std::string errMsg = fmt::format("'Start import at row' value ({}) is larger than the total number of lines in the file ({}).", wizardData.startImportRow, totalLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::START_LINE_LARGER_THAN_TOTAL), errMsg), {}};
  }

  if(wizardData.headersLine > totalLines)
  {
    std::string errMsg = fmt::format("Header 'Line Number' value ({}) is larger than the total number of lines in the file ({}).", wizardData.headersLine, totalLines);
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::HEADER_LINE_LARGER_THAN_TOTAL), errMsg), {}};
  }

  if(headerMode == CSVWizardData::HeaderMode::CUSTOM)
  {
    for(int i = 0; i < wizardData.customHeaders.size(); i++)
    {
      const auto& customHeader = wizardData.customHeaders[i];
      if(customHeader.empty())
      {
        std::string errMsg = fmt::format("The header for column #{} is empty.  Please fill in a header for column #{}.", i + 1, i + 1);
        return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_NAMES), errMsg), {}};
      }

      for(int j = 0; j < wizardData.customHeaders.size(); j++)
      {
        std::string otherHeader = wizardData.customHeaders[j];

        if(i != j && !customHeader.empty() && !otherHeader.empty() && customHeader == otherHeader)
        {
          std::string errMsg = fmt::format("Header '{}' (column #{}) and header '{}' (column #{}) have the same name.  Headers may not have duplicate names.", customHeader, i + 1, otherHeader, j + 1);
          return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::DUPLICATE_NAMES), errMsg), {}};
        }
      }
    }
  }

  // Create the arrays
  for(usize i = 0; i < wizardData.dataTypes.size() && i < headers.size(); i++)
  {
    if(wizardData.skippedArrayMask[i])
    {
      // The user decided to skip importing this array
      continue;
    }

    DataType dataType = wizardData.dataTypes[i];
    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);

    std::vector<usize> tupleDimsUSize(wizardData.tupleDims.size());
    std::transform(wizardData.tupleDims.begin(), wizardData.tupleDims.end(), tupleDimsUSize.begin(), [](float64 d) { return static_cast<usize>(d); });
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tupleDimsUSize, cDims, arrayPath));
  }

  // Create preflight updated values
  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Input File Path", wizardData.inputFilePath});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportCSVDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  CSVWizardData wizardData = filterArgs.value<CSVWizardData>(k_WizardData_Key);
  bool useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedDataGroup = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  DataPath createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = wizardData.inputFilePath;
  StringVector headers = s_HeaderCache[s_InstanceId].Headers;
  DataTypeVector dataTypes = wizardData.dataTypes;
  bool consecutiveDelimiters = wizardData.consecutiveDelimiters;
  usize startImportRow = wizardData.startImportRow;

  DataPath groupPath = createdDataGroup;
  if(useExistingGroup)
  {
    groupPath = selectedDataGroup;
  }

  Result<ParsersVector> parsersResult = createParsers(dataTypes, groupPath, headers, dataStructure);
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

  CharVector delimiters;
  if(wizardData.semicolonAsDelimiter)
  {
    delimiters.push_back(';');
  }
  if(wizardData.spaceAsDelimiter)
  {
    delimiters.push_back(' ');
  }
  if(wizardData.commaAsDelimiter)
  {
    delimiters.push_back(',');
  }
  if(wizardData.tabAsDelimiter)
  {
    delimiters.push_back('\t');
  }

  float32 threshold = 0.0f;
  usize numTuples = std::accumulate(wizardData.tupleDims.begin(), wizardData.tupleDims.end(), 1, std::multiplies<>());
  usize lineNum = startImportRow;
  while(!in.eof())
  {
    if(shouldCancel)
    {
      return {};
    }

    Result<> parsingResult = parseLine(in, parsersResult.value(), delimiters, consecutiveDelimiters, lineNum, startImportRow);
    if(parsingResult.invalid())
    {
      return std::move(parsingResult);
    }

    notifyProgress(messageHandler, lineNum, numTuples, threshold);
    lineNum++;
  }

  return {};
}
} // namespace complex
