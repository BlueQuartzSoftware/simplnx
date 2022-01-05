#include "RawBinaryReaderFilter.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include "ComplexCore/Filters/Algorithms/RawBinaryReader.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{

int32 k_RbrZeroComponentsError = -391;
int32 k_RbrNumComponentsError = -392;
int32 k_RbrWrongType = -393;
int32 k_RbrEmptyFile = -394;
int32 k_RbrSkippedTooMuch = -395;

//------------------------------------------------------------------------------
std::string RawBinaryReaderFilter::name() const
{
  return FilterTraits<RawBinaryReaderFilter>::name.str();
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
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters RawBinaryReaderFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path(), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "", NumericType::int8));
  params.insert(std::make_unique<UInt64Parameter>(k_NumberOfComponents_Key, "Number of Components", "", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_Endian_Key, "Endian", "", 0, ChoicesParameter::Choices{"Little", "Big"}));
  params.insert(std::make_unique<UInt64Parameter>(k_SkipHeaderBytes_Key, "Skip Header Bytes", "", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "", DataPath(std::vector<std::string>{"Imported Array"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RawBinaryReaderFilter::clone() const
{
  return std::make_unique<RawBinaryReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RawBinaryReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<uint64>(k_NumberOfComponents_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  if(pNumberOfComponentsValue < 1)
  {
    return {MakeErrorResult<OutputActions>(k_RbrZeroComponentsError, "The number of components must be positive.")};
  }

  complex::Result<OutputActions> resultOutputActions;

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

  usize numTuples = totalElements / pNumberOfComponentsValue;

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    auto action = std::make_unique<CreateArrayAction>(pScalarTypeValue, std::vector<usize>{numTuples}, std::vector<usize>{pNumberOfComponentsValue}, pCreatedAttributeArrayPathValue);

    resultOutputActions.value().actions.push_back(std::move(action));
  }

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RawBinaryReaderFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  complex::RawBinaryReaderInputValues inputValues;

  inputValues.inputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.scalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  inputValues.numberOfComponentsValue = filterArgs.value<uint64>(k_NumberOfComponents_Key);
  inputValues.endianValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  inputValues.skipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  inputValues.createdAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  // Let the Algorithm instance do the work
  return RawBinaryReader(dataStructure, &inputValues, this, messageHandler)();
}
} // namespace complex
