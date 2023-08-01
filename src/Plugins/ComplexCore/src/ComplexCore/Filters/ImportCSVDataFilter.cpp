#include "ImportCSVDataFilter.hpp"

#include "ComplexCore/utils/CSVDataParser.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/ImportCSVDataParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fstream>

using namespace complex;

using ParsersVector = std::vector<std::unique_ptr<AbstractDataParser>>;
using StringVector = std::vector<std::string>;
using CharVector = std::vector<char>;
using DataTypeVector = std::vector<std::optional<DataType>>;
using Dimensions = std::vector<usize>;
namespace fs = std::filesystem;

namespace
{
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
  NEW_DG_EXISTS = -114
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
Result<OutputActions> validateTupleDimensions(const std::vector<usize>& tDims, usize totalLines)
{
  usize tupleTotal = std::accumulate(tDims.begin(), tDims.end(), static_cast<usize>(1), std::multiplies<usize>());
  if(tupleTotal != totalLines)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::INCORRECT_TUPLES),
                                           fmt::format("The current number of tuples ({}) do not match the total number of imported lines ({}).", tupleTotal, totalLines))};
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<OutputActions> validateExistingGroup(const DataPath& groupPath, const DataStructure& dataStructure, const std::vector<std::string>& headers)
{
  if(groupPath.empty())
  {
    return {MakeErrorResult<OutputActions>(to_underlying(IssueCodes::EMPTY_EXISTING_DG), "'Existing Data Group' - Data path is empty.")};
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
    std::optional<DataType> dataTypeOpt = dataTypes[i];
    if(!dataTypeOpt.has_value())
    {
      continue;
    }

    std::string name = headers[i];

    DataPath arrayPath = parentPath;
    arrayPath = arrayPath.createChildPath(name);

    DataType dataType = dataTypeOpt.value();
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

  if(dataParsers.size() > tokens.size())
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
void skipNumberOfLines(std::fstream& inStream, usize numberOfLines)
{
  for(usize i = 1; i < numberOfLines; i++)
  {
    std::string line;
    std::getline(inStream, line);
  }
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ImportCSVDataFilter::ImportCSVDataFilter() = default;

// -----------------------------------------------------------------------------
ImportCSVDataFilter::~ImportCSVDataFilter() noexcept = default;

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
  return {"IO", "Input", "Read", "Import", "ASCII", "ascii", "CSV", "csv", "Column"};
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
  params.insert(
      std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "CSV Tuple Dimensions", "The tuple dimensions for the imported CSV data arrays", DynamicTableInfo::TableDataType{{1.0}}, tableInfo));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseExistingGroup_Key, "Use Existing Group", "Store the imported CSV data arrays in an existing group.", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataGroup_Key, "Existing Data Group", "Store the imported CSV data arrays in this existing group.", DataPath{},
                                                              BaseGroup::GetAllGroupTypes()));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDataGroup_Key, "New Data Group", "Store the imported CSV data arrays in a newly created group.", DataPath{}));

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
IFilter::PreflightResult ImportCSVDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  CSVWizardData wizardData = filterArgs.value<CSVWizardData>(k_WizardData_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);
  bool useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedDataGroup = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  DataPath createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = wizardData.inputFilePath;
  StringVector headers = wizardData.dataHeaders;
  DataTypeVector dataTypes = wizardData.dataTypes;
  Dimensions cDims = {1};
  complex::Result<OutputActions> resultOutputActions;

  // Validate the input file path
  Result<OutputActions> result = validateInputFilePath(inputFilePath);
  if(result.invalid())
  {
    return {std::move(result)};
  }

  // Validate the tuple dimensions
  const auto& row = tableData.at(0);
  std::vector<usize> tDims;
  tDims.reserve(row.size());
  for(auto value : row)
  {
    tDims.push_back(static_cast<usize>(value));
  }
  usize totalLines = wizardData.numberOfLines - wizardData.beginIndex + 1;
  result = validateTupleDimensions(tDims, totalLines);
  if(result.invalid())
  {
    return {std::move(result)};
  }

  // Validate the existing/created group
  DataPath groupPath;
  if(useExistingGroup)
  {
    result = validateExistingGroup(selectedDataGroup, dataStructure, headers);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = selectedDataGroup;
  }
  else
  {
    result = validateNewGroup(createdDataGroup, dataStructure);
    if(result.invalid())
    {
      return {std::move(result)};
    }
    groupPath = createdDataGroup;
    resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(createdDataGroup));
  }

  // Create the arrays
  for(usize i = 0; i < dataTypes.size() && i < headers.size(); i++)
  {
    std::optional<DataType> dataTypeOpt = dataTypes[i];
    if(!dataTypeOpt.has_value())
    {
      // This data type optional does not have a value because the user decided to skip importing this array
      continue;
    }

    DataType dataType = dataTypeOpt.value();
    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);

    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tDims, cDims, arrayPath));
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
  StringVector headers = wizardData.dataHeaders;
  DataTypeVector dataTypes = wizardData.dataTypes;
  CharVector delimiters = wizardData.delimiters;
  bool consecutiveDelimiters = wizardData.consecutiveDelimiters;
  usize numLines = wizardData.numberOfLines;
  usize beginIndex = wizardData.beginIndex;

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

  ParsersVector dataParsers = std::move(parsersResult.value());

  std::fstream in(inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  // Skip to the first data line
  skipNumberOfLines(in, beginIndex);

  float32 threshold = 0.0f;
  usize numTuples = numLines - beginIndex + 1;
  for(usize lineNum = beginIndex; lineNum <= numLines; lineNum++)
  {
    if(shouldCancel)
    {
      return {};
    }

    Result<> parsingResult = parseLine(in, dataParsers, delimiters, consecutiveDelimiters, lineNum, beginIndex);
    if(parsingResult.invalid())
    {
      return std::move(parsingResult);
    }

    notifyProgress(messageHandler, lineNum, numTuples, threshold);
  }

  return {};
}
} // namespace complex
