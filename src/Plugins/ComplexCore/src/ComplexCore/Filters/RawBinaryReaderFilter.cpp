#include "RawBinaryReaderFilter.hpp"

#include "ComplexCore/Filters/Algorithms/RawBinaryReader.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace complex
{
constexpr int32 k_RbrZeroComponentsError = -391;
constexpr int32 k_RbrNumComponentsError = -392;
constexpr int32 k_RbrWrongType = -393;
constexpr int32 k_RbrEmptyFile = -394;
constexpr int32 k_RbrSkippedTooMuch = -395;
constexpr int32 k_RbrTupleDimsInconsistent = -397;

//------------------------------------------------------------------------------
std::string RawBinaryReaderFilter::name() const
{
  return FilterTraits<RawBinaryReaderFilter>::name;
}

//------------------------------------------------------------------------------
std::string RawBinaryReaderFilter::className() const
{
  return FilterTraits<RawBinaryReaderFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RawBinaryReaderFilter::uuid() const
{
  return FilterTraits<RawBinaryReaderFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RawBinaryReaderFilter::humanName() const
{
  return "Raw Binary Importer";
}

//------------------------------------------------------------------------------
std::vector<std::string> RawBinaryReaderFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters RawBinaryReaderFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input binary file path", fs::path(), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "Data type of the binary data", NumericType::int8));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo{1, "Value {}"});
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Dim 0"}));
  params.insert(
      std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Dimensions", "Slowest to Fastest Dimensions (ZYX for example)", DynamicTableInfo::TableDataType{{1.0}}, tableInfo));
  params.insert(std::make_unique<UInt64Parameter>(k_NumberOfComponents_Key, "Number of Components", "The number of values at each tuple", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_Endian_Key, "Endian", "The endianness of the data", 0, ChoicesParameter::Choices{"Little", "Big"}));
  params.insert(std::make_unique<UInt64Parameter>(k_SkipHeaderBytes_Key, "Skip Header Bytes", "Number of bytes to skip before reading data", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "The complete path to the created Attribute Array",
                                                         DataPath(std::vector<std::string>{"Imported Array"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RawBinaryReaderFilter::clone() const
{
  return std::make_unique<RawBinaryReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RawBinaryReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<uint64>(k_NumberOfComponents_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);
  auto pTupleDimsValue = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  if(pNumberOfComponentsValue < 1)
  {
    return {MakeErrorResult<OutputActions>(k_RbrZeroComponentsError, "The number of components must be positive.")};
  }

  const auto& rowData = pTupleDimsValue.at(0);
  std::vector<usize> tupleDims;
  for(auto floatValue : rowData)
  {
    tupleDims.push_back(static_cast<usize>(floatValue));
  }

  Result<OutputActions> resultOutputActions;

  usize inputFileSize = fs::file_size(pInputFileValue);
  if(inputFileSize == 0)
  {
    return {MakeErrorResult<OutputActions>(k_RbrEmptyFile, fmt::format("File '{}' is empty.", pInputFileValue.string()))};
  }

  usize totalBytesToRead = inputFileSize - pSkipHeaderBytesValue;
  if(totalBytesToRead <= 0)
  {
    return {MakeErrorResult<OutputActions>(k_RbrSkippedTooMuch, fmt::format("More bytes have been skipped than exist in file '{}'.", pInputFileValue.string()))};
  }

  usize typeSize = GetNumericTypeSize(pScalarTypeValue);

  // Sanity check the chosen scalar type and the total bytes in the data file
  if(totalBytesToRead % typeSize != 0)
  {
    return {MakeErrorResult<OutputActions>(
        k_RbrWrongType,
        fmt::format(
            "After skipping {} bytes, the data to be read in file '{}' does not convert into an exact number of elements using the chosen scalar type. Are you sure this is the correct scalar type?",
            pSkipHeaderBytesValue, pInputFileValue.string()))};
  }

  usize totalElements = totalBytesToRead / typeSize;

  // Sanity check the chosen number of components and the number of elements in the data file
  if(totalElements % pNumberOfComponentsValue != 0)
  {
    return {MakeErrorResult<OutputActions>(
        k_RbrNumComponentsError,
        fmt::format("After skipping {} bytes, the data in file '{}' does not convert into an exact number of tuples using the chosen components.  Are you sure this is the correct component number?",
                    std::to_string(pSkipHeaderBytesValue), pInputFileValue.string()))};
  }

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), tupleDims, std::vector<usize>{pNumberOfComponentsValue}, pCreatedAttributeArrayPathValue);

    resultOutputActions.value().appendAction(std::move(action));
  }

  usize numTuples = totalElements / pNumberOfComponentsValue;
  size_t tupleCountFromTable = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<size_t>(1), std::multiplies<size_t>());
  if(numTuples != tupleCountFromTable)
  {
    const std::string warningMessage = fmt::format("Total Tuples based on file '{}' does not match total tuples entered. '{}'. Reading a subsection of the file.", numTuples, tupleCountFromTable);

    resultOutputActions.warnings().push_back({k_RbrTupleDimsInconsistent, warningMessage});
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> RawBinaryReaderFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  RawBinaryReaderInputValues inputValues;

  inputValues.inputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.scalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  inputValues.numberOfComponentsValue = filterArgs.value<uint64>(k_NumberOfComponents_Key);
  inputValues.endianValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  inputValues.skipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  inputValues.createdAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  // Let the Algorithm instance do the work
  return RawBinaryReader(dataStructure, inputValues, shouldCancel, messageHandler)();
}
} // namespace complex
