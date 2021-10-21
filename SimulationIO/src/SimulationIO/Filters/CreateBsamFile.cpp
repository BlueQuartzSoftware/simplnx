#include "CreateBsamFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateBsamFile::name() const
{
  return FilterTraits<CreateBsamFile>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateBsamFile::className() const
{
  return FilterTraits<CreateBsamFile>::className;
}

//------------------------------------------------------------------------------
Uuid CreateBsamFile::uuid() const
{
  return FilterTraits<CreateBsamFile>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateBsamFile::humanName() const
{
  return "Create BSAM File";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateBsamFile::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters CreateBsamFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputFilePrefix_Key, "Output File Prefix", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_NumClusters_Key, "Number of Clusters", "", 1234356));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateBsamFile::clone() const
{
  return std::make_unique<CreateBsamFile>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CreateBsamFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pNumClustersValue = filterArgs.value<int32>(k_NumClusters_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateBsamFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateBsamFile::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pNumClustersValue = filterArgs.value<int32>(k_NumClusters_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
