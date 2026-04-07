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

  params.insertSeparator(Parameters::Separator{"Tuple Dimensions"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an existing Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "TUPLE DIM {}"));
    const DynamicTableInfo::TableDataType defaultTable{{1.0F}};
    params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Tuple Dimensions (Slowest to Fastest Dimensions)",
                                                          "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", defaultTable, tableInfo));
  }

  params.insertSeparator(Parameters::Separator{"Component Dimensions"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "COMP DIM {}"));
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
  auto useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  auto pCompDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_CompDims_Key);
  auto pTupleDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  // Validate component dimensions
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

  Result<OutputActions> resultOutputActions;

  // Resolve tuple dimensions (AttributeMatrix-aware)
  ShapeType tupleDims;
  auto* parentAM = dataStructure.getDataAs<AttributeMatrix>(pCreatedAttributeArrayPathValue.getParent());

  if(parentAM != nullptr)
  {
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back(
          {-78702, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. "
                   "The Attribute Matrix tuples will override your custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }
  else
  {
    if(!useDims)
    {
      return {MakeErrorResult<OutputActions>(-78703, fmt::format("The DataArray to be created '{}' is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. "
                                                                 "Check Set Tuple Dimensions to set the dimensions.",
                                                                 pCreatedAttributeArrayPathValue.toString()))};
    }
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

  // Validate file size
  usize inputFileSize = fs::file_size(pInputFileValue);
  if(inputFileSize == 0)
  {
    return {MakeErrorResult<OutputActions>(-78705, fmt::format("File '{}' is empty.", pInputFileValue.string()))};
  }

  if(pSkipHeaderBytesValue >= inputFileSize)
  {
    return {MakeErrorResult<OutputActions>(-78706, fmt::format("Skip Header Bytes ({}) is greater than or equal to the file size ({}) for file '{}'.", pSkipHeaderBytesValue, inputFileSize,
                                                               pInputFileValue.string()))};
  }

  usize totalBytesToRead = inputFileSize - pSkipHeaderBytesValue;
  usize typeSize = GetNumericTypeSize(pScalarTypeValue);

  if(totalBytesToRead % typeSize != 0)
  {
    return {MakeErrorResult<OutputActions>(
        -78707, fmt::format("After skipping {} bytes, the data in file '{}' does not convert into an exact number of elements using the chosen scalar type '{}'. "
                            "Are you sure this is the correct scalar type?",
                            pSkipHeaderBytesValue, pInputFileValue.string(), DataTypeToString(ConvertNumericTypeToDataType(pScalarTypeValue))))};
  }

  // Validate data fits
  usize totalElementsInFile = totalBytesToRead / typeSize;
  usize requiredElements = numTuples * numComponents;

  if(requiredElements > totalElementsInFile)
  {
    return {MakeErrorResult<OutputActions>(
        -78708, fmt::format("The file does not contain enough data for the requested array dimensions. "
                            "Required elements: {} (tuples: {} x components: {}). Available elements in file: {}.",
                            requiredElements, numTuples, numComponents, totalElementsInFile))};
  }

  if(requiredElements < totalElementsInFile)
  {
    resultOutputActions.warnings().push_back(
        {-78709, fmt::format("Only a subset of the file data will be read. Required elements: {} (tuples: {} x components: {}). Available elements in file: {}.", requiredElements, numTuples,
                             numComponents, totalElementsInFile)});
  }

  // Create output array action
  {
    auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), tupleDims, compDims, pCreatedAttributeArrayPathValue);
    resultOutputActions.value().appendAction(std::move(action));
  }

  return {std::move(resultOutputActions)};
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

//------------------------------------------------------------------------------
Result<Arguments> ReadRawBinaryFilter::fromJson(const nlohmann::json& json) const
{
  // Delegate to base class for now; Task 6 will implement version migration
  return IFilter::fromJson(json);
}

Result<Arguments> ReadRawBinaryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadRawBinaryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::NumericTypeParameterConverter>(args, json, SIMPL::k_ScalarTypeKey, k_ScalarType_Key));
  // Note: k_NumberOfComponentsKey from SIMPL is migrated to k_CompDims_Key via fromJson() version migration
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_EndianKey, k_Endian_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToIntFilterParameterConverter<uint64>>(args, json, SIMPL::k_SkipHeaderBytesKey, k_SkipHeaderBytes_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedAttributeArrayPathKey, k_CreatedAttributeArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
