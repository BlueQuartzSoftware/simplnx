#include "ReadCSVFileFilter.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadCSVFile.hpp"

#include <fstream>

using namespace nx::core;

using Dimensions = std::vector<usize>;
namespace fs = std::filesystem;

namespace
{
struct ReadCSVFileFilterCache
{
  std::string FilePath;
  usize TotalLines = 0;
  usize HeadersLine = 0;
  std::vector<std::string> Headers;
  fs::file_time_type LastModifiedTime;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ReadCSVFileFilterCache> s_HeaderCache;

// -----------------------------------------------------------------------------
Result<OutputActions> validateExistingGroup(const DataPath& groupPath, const DataStructure& dataStructure, const std::vector<std::string>& headers)
{
  if(groupPath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::EMPTY_EXISTING_DG), "'Existing Data Group or Attribute Matrix' - Data path is empty.")};
  }

  const auto& selectedGroup = dataStructure.getDataRefAs<BaseGroup>(groupPath);
  const auto arrays = selectedGroup.findAllChildrenOfType<IDataArray>();
  for(const std::shared_ptr<IDataArray>& array : arrays)
  {
    std::string arrayName = array->getName();
    for(const std::string& headerName : headers)
    {
      if(arrayName == headerName)
      {
        return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::DUPLICATE_NAMES),
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
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::EMPTY_NEW_DG), "'New Data Group' - Data path is empty.")};
  }

  if(dataStructure.getData(groupPath) != nullptr)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::NEW_DG_EXISTS),
                                           fmt::format("The group at the path '{}' cannot be created because it already exists.", groupPath.toString()))};
  }

  return {};
}

//------------------------------------------------------------------------------
IFilter::PreflightResult readHeaders(const std::string& inputFilePath, usize headersLineNum, const std::vector<char>& delimiters, bool consecutiveDelimiters, ReadCSVFileFilterCache& headerCache)
{
  auto result = FileUtilities::CSV::ReadHeaders(inputFilePath, headersLineNum, delimiters, consecutiveDelimiters);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(result))), {})};
  }

  // Read the headers line
  headerCache.Headers = result.value();
  headerCache.HeadersLine = headersLineNum;
  return {};
}

Result<> cacheHeaders(int32 instanceId, const ReadCSVData& readCsvData)
{
  std::fstream in(readCsvData.inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(to_underlying(ReadCSVFile::IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", readCsvData.inputFilePath));
  }

  usize currentLine = 0;
  while(!in.eof())
  {
    std::string line;
    std::getline(in, line);
    currentLine++;

    if(currentLine == readCsvData.headersLine)
    {
      auto headers = StringUtilities::split(line, readCsvData.delimiters, readCsvData.consecutiveDelimiters);
      s_HeaderCache[instanceId].Headers = headers;
      s_HeaderCache[instanceId].HeadersLine = readCsvData.headersLine;
      break;
    }
  }

  return {};
}

Result<> cacheFullFile(int32 instanceId, const ReadCSVData& readCsvData)
{
  s_HeaderCache[instanceId].FilePath = readCsvData.inputFilePath;
  if(readCsvData.headerMode == ReadCSVData::HeaderMode::LINE && readCsvData.headersLine != s_HeaderCache[instanceId].HeadersLine)
  {
    auto result = cacheHeaders(instanceId, readCsvData);
    if(result.invalid())
    {
      return result;
    }
  }

  s_HeaderCache[instanceId].TotalLines = nx::core::FileUtilities::LinesInFile(readCsvData.inputFilePath);
  s_HeaderCache[instanceId].LastModifiedTime = fs::last_write_time(readCsvData.inputFilePath);

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

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<ReadCSVFileParameter>(k_ReadCSVData_Key, "CSV Importer Data", "Holds all relevant csv file data collected from the custom interface", ReadCSVData()));

  params.insertSeparator(Parameters::Separator{"Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseExistingGroup_Key, "Use Existing Data Group or Attribute Matrix",
                                                                 "Store the imported CSV data arrays in an existing data group or attribute matrix.", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Existing Data Group or Attribute Matrix",
                                                              "Store the imported CSV data arrays in an existing data group or attribute matrix.", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix, BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDataGroup_Key, "New Attribute Matrix", "Store the imported CSV data arrays in a newly created attribute matrix.",
                                                             DataPath{{"Imported Data"}}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseExistingGroup_Key, k_SelectedAttributeMatrixPath_Key, true);
  params.linkParameters(k_UseExistingGroup_Key, k_CreatedDataGroup_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadCSVFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadCSVFileFilter::clone() const
{
  return std::make_unique<ReadCSVFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadCSVFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto readCSVData = filterArgs.value<ReadCSVData>(k_ReadCSVData_Key);
  auto useExistingGroupOrAM = filterArgs.value<bool>(k_UseExistingGroup_Key);
  auto selectedGroupOrAM = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto createdDataAM = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = readCSVData.inputFilePath;
  ReadCSVData::HeaderMode headerMode = readCSVData.headerMode;

  nx::core::Result<OutputActions> resultOutputActions;

  // Validate the input file path
  if(inputFilePath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::EMPTY_FILE), "A file has not been chosen to import. Please pick a file to import.")};
  }

  Result<> csvResult = FileUtilities::ValidateCSVFile(inputFilePath);
  if(csvResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(csvResult)), {}), {}};
  }

  std::vector<std::string> headers;
  auto lastModifiedTime = fs::last_write_time(readCSVData.inputFilePath);
  if(readCSVData.inputFilePath != s_HeaderCache[m_InstanceId].FilePath || lastModifiedTime != s_HeaderCache[m_InstanceId].LastModifiedTime)
  {
    // File path changed or file was modified, so clear the cache and cache the full file again
    s_HeaderCache[m_InstanceId] = ReadCSVFileFilterCache{};
    auto result = cacheFullFile(m_InstanceId, readCSVData);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(result)), {})};
    }
  }
  else if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine != s_HeaderCache[m_InstanceId].HeadersLine)
  {
    // We are in header line mode and the header line number changed
    auto result = cacheHeaders(m_InstanceId, readCSVData);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(result)), {})};
    }
  }

  headers = (headerMode == ReadCSVData::HeaderMode::LINE) ? s_HeaderCache[m_InstanceId].Headers : readCSVData.customHeaders;
  usize totalLines = s_HeaderCache[m_InstanceId].TotalLines;

  // Check that we have a valid start import row
  if(readCSVData.startImportRow == 0)
  {
    std::string errMsg = "'Start import at row' value is out of range.  The 'Start import at row' value cannot be set to line #0.";
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::START_IMPORT_ROW_OUT_OF_RANGE), errMsg), {}};
  }

  if(readCSVData.startImportRow > totalLines)
  {
    std::string errMsg = fmt::format("'Start import at row' value ({}) is larger than the total number of lines in the file ({}).", readCSVData.startImportRow, totalLines);
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::START_IMPORT_ROW_OUT_OF_RANGE), errMsg), {}};
  }

  // Check that we have a valid header line number
  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine == 0)
  {
    std::string errMsg = "The header line number is out of range.  The header line number cannot be set to line #0.";
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine > totalLines)
  {
    std::string errMsg = fmt::format("The header line number is out of range.  There are {} lines in the file and the header line number is set to line #{}.", totalLines, readCSVData.headersLine);
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  if(headerMode == ReadCSVData::HeaderMode::LINE && readCSVData.headersLine > readCSVData.startImportRow)
  {
    std::string errMsg = fmt::format(
        "The header line number is out of range.  The start import row is set to line #{} and the header line number is set to line #{}.  The header line number must be in the range 1-{}.",
        readCSVData.startImportRow, readCSVData.headersLine, readCSVData.startImportRow - 1);
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::HEADER_LINE_OUT_OF_RANGE), errMsg), {}};
  }

  // Check that we have valid headers
  if(headers.empty())
  {
    std::string errMsg = "There are 0 imported array headers.  This is either because there are 0 headers being read from the header line or the custom headers are empty.  Please either choose a "
                         "different header line number or input at least 1 custom header.";
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::EMPTY_HEADERS), errMsg), {}};
  }

  if(readCSVData.dataTypes.size() != headers.size())
  {
    std::string errMsg =
        fmt::format("The number of data types ({}) does not match the number of imported array headers ({}).  The number of data types must match the number of imported array headers.",
                    readCSVData.dataTypes.size(), headers.size());
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::INCORRECT_DATATYPE_COUNT), errMsg), {}};
  }

  if(readCSVData.skippedArrayMask.size() != headers.size())
  {
    std::string errMsg = fmt::format(
        "The number of booleans in the skipped array mask ({}) does not match the number of imported array headers ({}).  The number of booleans in the skipped array mask must match the number "
        "of imported array headers.",
        readCSVData.skippedArrayMask.size(), headers.size());
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::INCORRECT_MASK_COUNT), errMsg), {}};
  }

  headers = FileUtilities::CSV::RemoveIllegalCharacters(headers);

  for(int i = 0; i < headers.size(); i++)
  {
    auto& headerName = headers[i];
    if(headerName.empty())
    {
      std::string errMsg = fmt::format("The header for column #{} is empty.  Please fill in a header for column #{}.", i + 1, i + 1);
      return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::EMPTY_NAMES), errMsg), {}};
    }

    if(StringUtilities::contains(headerName, '&') || StringUtilities::contains(headerName, ':') || StringUtilities::contains(headerName, '/') || StringUtilities::contains(headerName, '\\'))
    {
      return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::ILLEGAL_NAMES),
                                             fmt::format(R"(The header name "{}" contains a character that will cause problems. Do Not use '&',':', '/' or '\' in the header names.)", headerName))};
    }

    for(int j = 0; j < headers.size(); j++)
    {
      std::string otherHeaderName = headers[j];

      if(i != j && !headerName.empty() && !otherHeaderName.empty() && headerName == otherHeaderName)
      {
        std::string errMsg = fmt::format("Header '{}' (column #{}) and header '{}' (column #{}) have the same name.  Headers may not have duplicate names.", headerName, i + 1, otherHeaderName, j + 1);
        return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::DUPLICATE_NAMES), errMsg), {}};
      }
    }
  }

  // Check that we have a valid tuple count
  usize totalImportedLines = totalLines - readCSVData.startImportRow + 1;
  usize tupleTotal = std::accumulate(readCSVData.tupleDims.begin(), readCSVData.tupleDims.end(), static_cast<usize>(1), std::multiplies<>());
  if(tupleTotal == 0)
  {
    std::string tupleDimsStr = FileUtilities::CSV::TupleDimsToString(readCSVData.tupleDims);
    std::string errMsg = fmt::format("Error: The current tuple dimensions ({}) has 0 total tuples.  At least 1 tuple is required.", tupleDimsStr, tupleTotal, totalImportedLines);
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::INCORRECT_TUPLES), errMsg), {}};
  }
  else if(tupleTotal > totalImportedLines && !useExistingGroupOrAM)
  {
    std::string tupleDimsStr = FileUtilities::CSV::TupleDimsToString(readCSVData.tupleDims);
    std::string errMsg = fmt::format("Error: The current tuple dimensions ({}) has {} total tuples, but this is larger than the total number of available lines to import ({}).", tupleDimsStr,
                                     tupleTotal, totalImportedLines);
    return {MakeErrorResult<OutputActions>(to_underlying(ReadCSVFile::IssueCodes::INCORRECT_TUPLES), errMsg), {}};
  }

  // Validate the existing/created group
  DataPath groupPath;
  if(useExistingGroupOrAM)
  {
    Result<OutputActions> result = validateExistingGroup(selectedGroupOrAM, dataStructure, headers);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = selectedGroupOrAM;
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
  ShapeType tupleDims(readCSVData.tupleDims.size());
  std::transform(readCSVData.tupleDims.begin(), readCSVData.tupleDims.end(), tupleDims.begin(), [](usize d) { return d; });
  if(useExistingGroupOrAM)
  {
    const auto* am = dataStructure.getDataAs<AttributeMatrix>(groupPath);
    if(am != nullptr)
    {
      tupleDims = am->getShape();

      auto totalLinesRead = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<usize>(1), std::multiplies<>());

      std::string msg = fmt::format("The Array Tuple Dimensions ({}) will be ignored and the Existing Attribute Matrix tuple dimensions ({}) will be used. The total number of lines read will be {}.",
                                    fmt::join(readCSVData.tupleDims, "x"), fmt::join(tupleDims, "x"), totalLinesRead);
      resultOutputActions.warnings().push_back(Warning{to_underlying(ReadCSVFile::IssueCodes::IGNORED_TUPLE_DIMS), msg});
    }
  }

  for(usize i = 0; i < headers.size(); i++)
  {
    if(readCSVData.skippedArrayMask[i])
    {
      // The user decided to skip importing this array
      continue;
    }

    CSVType csvType = readCSVData.dataTypes[i];
    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);
    if(csvType == CSVType::string)
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(tupleDims, arrayPath));
    }
    else
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(ConvertCSVTypeToDataType(csvType), tupleDims, std::vector<usize>{1}, arrayPath));
    }
  }

  return {std::move(resultOutputActions), {}};
}

//------------------------------------------------------------------------------
Result<> ReadCSVFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto readCSVData = filterArgs.value<ReadCSVData>(k_ReadCSVData_Key);
  auto useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  auto selectedDataGroupOrAM = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::vector<std::string> headers = s_HeaderCache[m_InstanceId].Headers;

  if(readCSVData.headerMode == ReadCSVData::HeaderMode::CUSTOM)
  {
    headers = readCSVData.customHeaders;
  }

  DataPath groupPath = createdDataGroup;
  auto tupleDims = readCSVData.tupleDims;
  if(useExistingGroup)
  {
    groupPath = selectedDataGroupOrAM;
    const AttributeMatrix* am = dataStructure.getDataAs<AttributeMatrix>(groupPath);
    if(am != nullptr)
    {
      tupleDims = am->getShape();
    }
  }

  return ReadCSVFile().readFile(dataStructure, readCSVData.inputFilePath, readCSVData.startImportRow, headers, readCSVData.dataTypes, readCSVData.skippedArrayMask, groupPath, tupleDims,
                                readCSVData.delimiters, readCSVData.consecutiveDelimiters, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AutomaticAttrMatrixKey = "Wizard_AutomaticAM";
constexpr StringLiteral k_SelectedPathKey = "Wizard_SelectedPath";

std::vector<CSVType> ConvertCSVTypeStrings(const std::vector<std::string>& dataTypes)
{
  std::vector<CSVType> output;

  for(usize i = 0; i < dataTypes.size(); i++)
  {
    try
    {
      output.push_back(nx::core::StringToCSVType(dataTypes[i]));
    } catch(const std::exception& e)
    {
    }
  }

  return output;
}

std::vector<char> ConvertToChars(const std::string& string)
{
  return std::vector<char>(string.begin(), string.end());
}
} // namespace SIMPL
} // namespace

Result<Arguments> ReadCSVFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadCSVFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Convert the wizard data
  {
    auto result = SIMPLConversion::ReadASCIIWizardDataFilterParameterConverter::convert(json);
    if(result.valid())
    {
      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<typename SIMPLConversion::ReadASCIIWizardDataFilterParameterConverter::ValueType>(std::move(result.value())));
    }
    results.push_back(ConvertResult(std::move(result)));
  }

  // Convert the existing attr matrix boolean
  {
    auto result = SIMPLConversion::BooleanFilterParameterConverter::convert(json[SIMPL::k_AutomaticAttrMatrixKey]);
    if(result.invalid())
    {
      results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedPathKey, k_CreatedDataGroup_Key));
    }
    else
    {
      auto isCreatedDataGroup = result.value();
      if(isCreatedDataGroup)
      {
        results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixCreationFilterParameterConverter>(args, json, SIMPL::k_SelectedPathKey, k_CreatedDataGroup_Key));
      }
      else
      {
        results.push_back(
            SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedPathKey, k_SelectedAttributeMatrixPath_Key));
      }
    }
    results.push_back(ConvertResult(std::move(result)));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
