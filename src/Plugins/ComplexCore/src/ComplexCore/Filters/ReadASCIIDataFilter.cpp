#include "ReadASCIIDataFilter.hpp"

#include <fstream>

#include "complex/Common/Types.hpp"
#include "complex/Common/TypesUtility.hpp"
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
#include "complex/Parameters/ReadASCIIDataParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "ComplexCore/utils/ASCIIDataParser.hpp"

using namespace complex;

using StringList = std::list<std::string>;
using StringVector = std::vector<std::string>;
using CharVector = std::vector<char>;
using DataTypeVector = std::vector<std::optional<DataType>>;
using Dimensions = std::vector<usize>;
namespace fs = std::filesystem;

namespace
{
enum IssueCodes
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
}

namespace complex
{
//------------------------------------------------------------------------------
ReadASCIIDataFilter::ReadASCIIDataFilter()
{
}

// -----------------------------------------------------------------------------
ReadASCIIDataFilter::~ReadASCIIDataFilter() noexcept
{
}

std::string ReadASCIIDataFilter::name() const
{
  return FilterTraits<ReadASCIIDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadASCIIDataFilter::className() const
{
  return FilterTraits<ReadASCIIDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadASCIIDataFilter::uuid() const
{
  return FilterTraits<ReadASCIIDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadASCIIDataFilter::humanName() const
{
  return "Import ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadASCIIDataFilter::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ReadASCIIDataFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<ReadASCIIDataParameter>(k_WizardData_Key, "ASCII Wizard Data", "", ASCIIWizardData()));

  DynamicTableParameter::ValueType dynamicTable{{{1}}, {"Dim 0"}, {"Value"}};
  dynamicTable.setMinCols(1);
  dynamicTable.setDynamicCols(true);
  dynamicTable.setDynamicRows(false);
  params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "ASCII Tuple Dimensions", "The tuple dimensions for the imported ASCII data arrays", dynamicTable));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseExistingGroup_Key, "Use Existing Group", "Store the imported ASCII data arrays in an existing group.", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataGroup_Key, "Existing Data Group", "Store the imported ASCII data arrays in this existing group.", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedDataGroup_Key, "New Data Group", "Store the imported ASCII data arrays in a newly created group.", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseExistingGroup_Key, k_SelectedDataGroup_Key, true);
  params.linkParameters(k_UseExistingGroup_Key, k_CreatedDataGroup_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadASCIIDataFilter::clone() const
{
  return std::make_unique<ReadASCIIDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadASCIIDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  ASCIIWizardData wizardData = filterArgs.value<ASCIIWizardData>(k_WizardData_Key);
  DynamicTableData tupleDims = filterArgs.value<DynamicTableData>(k_TupleDims_Key);
  bool useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedDataGroup = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  DataPath createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  complex::Result<OutputActions> resultOutputActions;

  if(wizardData.inputFilePath().empty())
  {
    return {MakeErrorResult<OutputActions>(IssueCodes::EMPTY_FILE, "A file has not been chosen to import. Please pick a file to import.")};
  }

  std::string inputFilePath = wizardData.inputFilePath();
  StringVector headers = wizardData.dataHeaders();
  DataTypeVector dataTypes = wizardData.dataTypes();
  Dimensions cDims = {1};

  fs::path inputFile(inputFilePath);
  if(!fs::exists(inputFile))
  {
    return {MakeErrorResult<OutputActions>(IssueCodes::FILE_DOES_NOT_EXIST, fmt::format("The input file does not exist: '{}'", inputFilePath))};
  }

  // Validate the tuple dimensions
  auto tableData = tupleDims.getTableData();
  if(tableData.size() != 1)
  {
    return {MakeErrorResult<OutputActions>(IssueCodes::INCORRECT_TUPLES, fmt::format("The tuple dimensions table does not have exactly one row.  Something has gone horribly wrong."))};
  }

  std::vector<usize> tDims(tableData[0].begin(), tableData[0].end());
  usize tupleTotal = std::accumulate(tDims.begin(), tDims.end(), 1, std::multiplies<usize>());
  usize importedLines = wizardData.numberOfLines() - wizardData.beginIndex() + 1;
  if(tupleTotal != importedLines)
  {
    return {MakeErrorResult<OutputActions>(IssueCodes::INCORRECT_TUPLES,
                                           fmt::format("The current number of tuples ({}) do not match the total number of imported lines ({}).", tupleTotal, importedLines))};
  }

  DataPath groupPath;
  if(useExistingGroup)
  {
    groupPath = selectedDataGroup;
    if(selectedDataGroup.empty())
    {
      return {MakeErrorResult<OutputActions>(IssueCodes::EMPTY_EXISTING_DG, "'Existing Data Group' - Data path is empty.")};
    }

    const BaseGroup& selectedGroup = dataStructure.getDataRefAs<BaseGroup>(selectedDataGroup);
    const auto arrays = selectedGroup.findAllChildrenOfType<IDataArray>();
    for(const std::shared_ptr<IDataArray>& array : arrays)
    {
      std::string arrayName = array->getName();
      for(const std::string& headerName : headers)
      {
        if(arrayName == headerName)
        {
          return {MakeErrorResult<OutputActions>(IssueCodes::DUPLICATE_NAMES, fmt::format("The header name \"{}\" matches an array name that already exists in the selected container.", headerName))};
        }
        if(StringUtilities::contains(headerName, '&') || StringUtilities::contains(headerName, ':') || StringUtilities::contains(headerName, '/') || StringUtilities::contains(headerName, '\\'))
        {
          return {MakeErrorResult<OutputActions>(
              IssueCodes::ILLEGAL_NAMES, fmt::format("The header name \"{}\" contains a character that will cause problems. Do Not use '&',':', '/' or '\\' in the header names.", headerName))};
        }
      }
    }
  }
  else
  {
    groupPath = createdDataGroup;
    if(createdDataGroup.empty())
    {
      return {MakeErrorResult<OutputActions>(IssueCodes::EMPTY_NEW_DG, "'New Data Group' - Data path is empty.")};
    }

    if(dataStructure.getData(createdDataGroup) != nullptr)
    {
      return {MakeErrorResult<OutputActions>(IssueCodes::NEW_DG_EXISTS, fmt::format("The group at the path '{}' cannot be created because it already exists.", createdDataGroup.toString()))};
    }

    resultOutputActions.value().actions.push_back(std::make_unique<CreateDataGroupAction>(createdDataGroup));
  }

  // Create the arrays
  for(usize i = 0; i < dataTypes.size(); i++)
  {
    std::optional<DataType> dataTypeOpt = dataTypes[i];
    if(!dataTypeOpt.has_value())
    {
      continue;
    }

    DataType dataType = dataTypeOpt.value();
    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);

    resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(dataType, tDims, cDims, arrayPath));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Input File Path", wizardData.inputFilePath()});
  preflightUpdatedValues.push_back({"Total Tuples", std::to_string(wizardData.numberOfLines())});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadASCIIDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  ASCIIWizardData wizardData = filterArgs.value<ASCIIWizardData>(k_WizardData_Key);
  DynamicTableData tupleDims = filterArgs.value<DynamicTableData>(k_TupleDims_Key);
  bool useExistingGroup = filterArgs.value<bool>(k_UseExistingGroup_Key);
  DataPath selectedDataGroup = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  DataPath createdDataGroup = filterArgs.value<DataPath>(k_CreatedDataGroup_Key);

  std::string inputFilePath = wizardData.inputFilePath();
  StringVector headers = wizardData.dataHeaders();
  DataTypeVector dataTypes = wizardData.dataTypes();
  CharVector delimiters = wizardData.delimiters();
  bool consecutiveDelimiters = wizardData.consecutiveDelimiters();
  int numLines = wizardData.numberOfLines();
  int beginIndex = wizardData.beginIndex();

  std::vector<std::unique_ptr<AbstractDataParser>> dataParsers(dataTypes.size());

  DataPath groupPath = createdDataGroup;
  if(useExistingGroup)
  {
    groupPath = selectedDataGroup;
  }

  for(usize i = 0; i < dataTypes.size(); i++)
  {
    std::optional<DataType> dataTypeOpt = dataTypes[i];
    if(!dataTypeOpt.has_value())
    {
      continue;
    }

    std::string name = headers[i];

    DataPath arrayPath = groupPath;
    arrayPath = arrayPath.createChildPath(name);

    DataType dataType = dataTypeOpt.value();
    if(dataType == complex::DataType::float64)
    {
      Float64Array& data = dataStructure.getDataRefAs<Float64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float64Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::float32)
    {
      Float32Array& data = dataStructure.getDataRefAs<Float32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float32Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::int8)
    {
      Int8Array& data = dataStructure.getDataRefAs<Int8Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int8Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::int16)
    {
      Int16Array& data = dataStructure.getDataRefAs<Int16Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int16Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::int32)
    {
      Int32Array& data = dataStructure.getDataRefAs<Int32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int32Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::int64)
    {
      Int64Array& data = dataStructure.getDataRefAs<Int64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int64Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::uint8)
    {
      UInt8Array& data = dataStructure.getDataRefAs<UInt8Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt8Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::uint16)
    {
      UInt16Array& data = dataStructure.getDataRefAs<UInt16Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt16Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::uint32)
    {
      UInt32Array& data = dataStructure.getDataRefAs<UInt32Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt32Parser>(data, name, i);
    }
    else if(dataType == complex::DataType::uint64)
    {
      UInt64Array& data = dataStructure.getDataRefAs<UInt64Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt64Parser>(data, name, i);
    }
    //    else if(dataType == complex::DataType::string)
    //    {
    //      StringArray& data = dataStructure.getDataRefAs<StringArray>(arrayPath);
    //      dataParsers[i] = std::make_unique<StringParser>(data, name, i);
    //    }
    else
    {
      return {MakeErrorResult(IssueCodes::INVALID_ARRAY_TYPE, fmt::format("The data type that was chosen for column number {} is not a valid data array type.", std::to_string(i + 1)))};
    }
  }

  usize insertIndex = 0;

  std::fstream in(inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(IssueCodes::FILE_NOT_OPEN, fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  for(int i = 1; i < beginIndex; i++)
  {
    // Skip to the first data line
    std::string line;
    std::getline(in, line);
  }

  float threshold = 0.0f;
  size_t numTuples = numLines - beginIndex + 1;

  for(int lineNum = beginIndex; lineNum <= numLines; lineNum++)
  {
    std::string line;
    std::getline(in, line);

    StringVector tokens = StringUtilities::split(line, delimiters, consecutiveDelimiters);

    if(dataTypes.size() != tokens.size())
    {
      return MakeErrorResult(IssueCodes::INCONSISTENT_COLS, fmt::format("Line {} has an inconsistent number of columns.\nExpecting {} but found {}\nInput line was:\n{}", std::to_string(lineNum),
                                                                        std::to_string(dataTypes.size()), std::to_string(tokens.size()), line));
    }

    for(int i = 0; i < dataParsers.size(); i++)
    {
      if(dataParsers[i] == nullptr)
      {
        continue;
      }

      usize index = dataParsers[i]->columnIndex();

      Result<> result = dataParsers[i]->parse(tokens[index], insertIndex);
      if(result.invalid())
      {
        return result;
      }
    }

    const float percentCompleted = (static_cast<float>(lineNum) / numTuples) * 100.0f;
    if(percentCompleted > threshold)
    {
      // Print the status of the import
      messageHandler({IFilter::Message::Type::Info, fmt::format("Importing ASCII Data || {:.{}f}% Complete", static_cast<double>(percentCompleted), 1)});
      threshold = threshold + 5.0f;
      if(threshold < percentCompleted)
      {
        threshold = percentCompleted;
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    insertIndex++;
  }

  in.close();

  return {};
}
} // namespace complex
