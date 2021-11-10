#include "RawBinaryReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RawBinaryReader::name() const
{
  return FilterTraits<RawBinaryReader>::name.str();
}

//------------------------------------------------------------------------------
std::string RawBinaryReader::className() const
{
  return FilterTraits<RawBinaryReader>::className;
}

//------------------------------------------------------------------------------
Uuid RawBinaryReader::uuid() const
{
  return FilterTraits<RawBinaryReader>::uuid;
}

//------------------------------------------------------------------------------
std::string RawBinaryReader::humanName() const
{
  return "Raw Binary Importer";
}

//------------------------------------------------------------------------------
std::vector<std::string> RawBinaryReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters RawBinaryReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "", NumericType::int8));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfComponents_Key, "Number of Components", "", 1234356));
  params.insert(std::make_unique<ChoicesParameter>(k_Endian_Key, "Endian", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<UInt64Parameter>(k_SkipHeaderBytes_Key, "Skip Header Bytes", "", 13412341234212));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RawBinaryReader::clone() const
{
  return std::make_unique<RawBinaryReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RawBinaryReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pEndianValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RawBinaryReaderAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> RawBinaryReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pEndianValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
