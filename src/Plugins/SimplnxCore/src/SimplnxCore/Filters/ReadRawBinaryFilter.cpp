#include "ReadRawBinaryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadRawBinary.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
#include <numeric>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadRawBinaryFilter::name() const
{
  return FilterTraits<ReadRawBinaryFilter>::name;
}

//------------------------------------------------------------------------------
std::string ReadRawBinaryFilter::className() const
{
  return FilterTraits<ReadRawBinaryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadRawBinaryFilter::uuid() const
{
  return FilterTraits<ReadRawBinaryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadRawBinaryFilter::humanName() const
{
  return "Read Raw Binary";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadRawBinaryFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadRawBinaryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input binary file path", fs::path(), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Input Numeric Type", "Data type of the binary data", NumericType::int8));
  params.insert(std::make_unique<ChoicesParameter>(k_Endian_Key, "Endian", "The endianness of the data", 0, ChoicesParameter::Choices{"Little", "Big"}));
  params.insert(std::make_unique<UInt64Parameter>(k_SkipHeaderBytes_Key, "Skip Header Bytes", "Number of bytes to skip before reading data", 0));
  params.insert(std::make_unique<BoolParameter>(k_AllowPartialFilling_Key, "Allow Partial Filling of Array",
                                                "When enabled, the filter will read as much data as available and leave the remaining array elements default-initialized to 0", false));

  params.insertSeparator(Parameters::Separator{"Tuple Dimensions"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an existing Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, ""));
    const DynamicTableInfo::TableDataType defaultTable{{1.0F}};
    params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Tuple Dimensions (Slowest to Fastest Dimensions)",
                                                          "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", defaultTable, tableInfo));
  }

  params.insertSeparator(Parameters::Separator{"Component Dimensions"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, ""));
    const DynamicTableInfo::TableDataType defaultTable{{1.0F}};
    params.insert(std::make_unique<DynamicTableParameter>(k_CompDims_Key, "Data Array Component Dimensions (Slowest to Fastest Dimensions)", "Slowest to Fastest Component Dimensions.", defaultTable,
                                                          tableInfo));
  }

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "The complete path to the created Attribute Array",
                                                         DataPath(std::vector<std::string>{"Imported Array"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AdvancedOptions_Key, k_TupleDims_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadRawBinaryFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Change 1: k_NumberOfComponents_Key ("number_of_components") UInt64Parameter
  //            replaced with k_CompDims_Key ("component_dimensions") DynamicTableParameter
  // Change 2: Added k_AdvancedOptions_Key ("set_tuple_dimensions") BoolParameter
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadRawBinaryFilter::clone() const
{
  return std::make_unique<ReadRawBinaryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadRawBinaryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                            const ExecutionContext& executionContext) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);
  bool useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  bool allowPartialFilling = filterArgs.value<bool>(k_AllowPartialFilling_Key);
  auto pCompDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_CompDims_Key);
  auto pTupleDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::string scalarTypeString = DataTypeToString(ConvertNumericTypeToDataType(pScalarTypeValue));
  usize typeSize = GetNumericTypeSize(pScalarTypeValue);

  // ---- Check 1: File is empty ----
  usize inputFileSize = fs::file_size(pInputFileValue);
  if(inputFileSize == 0)
  {
    return {MakeErrorResult<OutputActions>(-78705, fmt::format("File '{}' is empty.", pInputFileValue.string()))};
  }

  // ---- Check 7: Component dimensions contain zero ----
  const std::vector<double>& compDimsRow = pCompDimsData.at(0);
  ShapeType compDims;
  compDims.reserve(compDimsRow.size());
  for(size_t idx = 0; idx < compDimsRow.size(); idx++)
  {
    if(compDimsRow[idx] == 0)
    {
      return {MakeErrorResult<OutputActions>(-78701, fmt::format("Component dimension at index {} cannot be 0", idx))};
    }
    compDims.push_back(static_cast<usize>(compDimsRow[idx]));
  }
  usize numComponents = std::accumulate(compDims.begin(), compDims.end(), static_cast<usize>(1), std::multiplies<>());

  // ---- Check 8/9/10: Resolve tuple dimensions (AttributeMatrix-aware) ----
  ShapeType tupleDims;
  auto* parentAM = dataStructure.getDataAs<AttributeMatrix>(pCreatedAttributeArrayPathValue.getParent());

  if(parentAM != nullptr)
  {
    // Check 10: AM parent + user dims checked — warn that AM shape overrides
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back({-78702,
                                                "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. "
                                                "The Attribute Matrix tuples will override your custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }
  else
  {
    // Check 9: Not in AM + dims unchecked — error
    if(!useDims)
    {
      return {MakeErrorResult<OutputActions>(-78703, fmt::format("The DataArray to be created '{}' is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. "
                                                                 "Check Set Tuple Dimensions to set the dimensions.",
                                                                 pCreatedAttributeArrayPathValue.toString()))};
    }
    // Check 8: Tuple dimensions contain zero
    const std::vector<double>& tupleDimsRow = pTupleDimsData.at(0);
    tupleDims.reserve(tupleDimsRow.size());
    for(size_t idx = 0; idx < tupleDimsRow.size(); idx++)
    {
      if(tupleDimsRow[idx] == 0)
      {
        return {MakeErrorResult<OutputActions>(-78704, fmt::format("Tuple dimension at index {} cannot be 0", idx))};
      }
      tupleDims.push_back(static_cast<usize>(tupleDimsRow[idx]));
    }
  }

  usize numTuples = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<usize>(1), std::multiplies<>());

  // ---- Check 2: Skip header bytes >= file size ----
  if(pSkipHeaderBytesValue >= inputFileSize)
  {
    return {MakeErrorResult<OutputActions>(
        -78706, fmt::format("Skip Header Bytes ({}) is greater than or equal to the file size ({}) for file '{}'.", pSkipHeaderBytesValue, inputFileSize, pInputFileValue.string()))};
  }

  // ---- Data-fit checks (4/5/6/11) ----
  usize availableBytes = inputFileSize - pSkipHeaderBytesValue;
  usize availableElements = availableBytes / typeSize;
  usize remainderBytes = availableBytes % typeSize;
  usize requiredElements = numTuples * numComponents;

  // Check 11: Remainder bytes warning (available bytes not evenly divisible by type size)
  if(remainderBytes != 0)
  {
    resultOutputActions.warnings().push_back(
        {-78707, fmt::format("{} bytes remain after reading whole {} elements from file '{}'. Verify the scalar type is correct.", remainderBytes, scalarTypeString, pInputFileValue.string())});
  }

  // Check 4: File doesn't have enough data for the requested array
  if(requiredElements > availableElements)
  {
    if(!allowPartialFilling)
    {
      return {MakeErrorResult<OutputActions>(-78708, fmt::format("The file does not contain enough data for the requested array dimensions. "
                                                                 "Required elements: {} (tuples: {} x components: {}). Available elements in file: {}. "
                                                                 "Enable 'Allow Partial Filling of Array' to read available data and default-initialize the remainder.",
                                                                 requiredElements, numTuples, numComponents, availableElements))};
    }
    resultOutputActions.warnings().push_back(
        {-78710, fmt::format("The file contains only {} of the {} required elements. The remaining elements will be default-initialized to 0.", availableElements, requiredElements)});
  }

  // Check 6: Only a subset of the file data will be read
  if(requiredElements < availableElements)
  {
    resultOutputActions.warnings().push_back({-78709, fmt::format("Only a subset of the file data will be read. Required elements: {} (tuples: {} x components: {}). Available elements in file: {}.",
                                                                  requiredElements, numTuples, numComponents, availableElements)});
  }

  // Create output array action
  {
    auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), tupleDims, compDims, pCreatedAttributeArrayPathValue);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Build preflight summary
  std::string summary = fmt::format("Reading {} tuples of {} with {} component(s) into '{}'.", numTuples, scalarTypeString, numComponents, pCreatedAttributeArrayPathValue.toString());

  if(requiredElements < availableElements)
  {
    summary += fmt::format(" Only {} of {} available elements will be read from the file.", requiredElements, availableElements);
  }
  else if(requiredElements > availableElements && allowPartialFilling)
  {
    summary += fmt::format(" The file contains only {} of the {} required elements. The remaining elements will be default-initialized to 0.", availableElements, requiredElements);
  }

  if(remainderBytes != 0)
  {
    summary += fmt::format(" Note: {} bytes remain after reading whole {} elements — verify the scalar type is correct.", remainderBytes, scalarTypeString);
  }

  preflightUpdatedValues.push_back({"Read Summary", summary});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadRawBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadRawBinaryInputValues inputValues;

  inputValues.inputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.scalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto compDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_CompDims_Key);
  const std::vector<double>& compDimsRow = compDimsData.at(0);
  ShapeType compDims;
  compDims.reserve(compDimsRow.size());
  for(size_t idx = 0; idx < compDimsRow.size(); idx++)
  {
    compDims.push_back(static_cast<usize>(compDimsRow[idx]));
  }
  inputValues.componentDimsValue = compDims;
  inputValues.endianValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  inputValues.skipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  inputValues.createdAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  // Let the Algorithm instance do the work
  return ReadRawBinary(dataStructure, inputValues, shouldCancel, messageHandler)();
}

//------------------------------------------------------------------------------
Result<Arguments> ReadRawBinaryFilter::fromJson(const nlohmann::json& json) const
{
  auto version = json.value("parameters_version", 1);
  if(version < 2)
  {
    nlohmann::json migrated = json;
    // Convert old UInt64 "number_of_components" to new DynamicTable "component_dimensions"
    if(migrated.contains("number_of_components"))
    {
      uint64 numComp = migrated["number_of_components"].get<uint64>();
      migrated["component_dimensions"] = DynamicTableParameter::ValueType{{static_cast<double>(numComp)}};
      migrated.erase("number_of_components");
    }
    // Add defaults for new parameters
    if(!migrated.contains("set_tuple_dimensions"))
    {
      migrated["set_tuple_dimensions"] = true;
    }
    if(!migrated.contains("allow_partial_filling"))
    {
      migrated["allow_partial_filling"] = false;
    }
    migrated["parameters_version"] = 2;
    return IFilter::fromJson(migrated);
  }
  return IFilter::fromJson(json);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_ScalarTypeKey = "ScalarType";
constexpr StringLiteral k_NumberOfComponentsKey = "NumberOfComponents";
constexpr StringLiteral k_EndianKey = "Endian";
constexpr StringLiteral k_SkipHeaderBytesKey = "SkipHeaderBytes";
constexpr StringLiteral k_CreatedAttributeArrayPathKey = "CreatedAttributeArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadRawBinaryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadRawBinaryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::NumericTypeParameterConverter>(args, json, SIMPL::k_ScalarTypeKey, k_ScalarType_Key));

  // Convert old integer NumberOfComponents to DynamicTable component_dimensions
  if(json.contains(SIMPL::k_NumberOfComponentsKey))
  {
    try
    {
      uint64 numComp = json[SIMPL::k_NumberOfComponentsKey].get<uint64>();
      args.insertOrAssign(k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<double>(numComp)}}));
    } catch(const nlohmann::json::exception& e)
    {
      results.push_back(MakeErrorResult(-1, fmt::format("Failed to convert NumberOfComponents from SIMPL JSON: {}", e.what())));
    }
  }

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_EndianKey, k_Endian_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToIntFilterParameterConverter<uint64>>(args, json, SIMPL::k_SkipHeaderBytesKey, k_SkipHeaderBytes_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedAttributeArrayPathKey, k_CreatedAttributeArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
