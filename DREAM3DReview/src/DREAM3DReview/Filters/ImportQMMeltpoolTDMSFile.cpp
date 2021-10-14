#include "ImportQMMeltpoolTDMSFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ImportQMMeltpoolTDMSFile::name() const
{
  return FilterTraits<ImportQMMeltpoolTDMSFile>::name.str();
}

std::string ImportQMMeltpoolTDMSFile::className() const
{
  return FilterTraits<ImportQMMeltpoolTDMSFile>::className;
}

Uuid ImportQMMeltpoolTDMSFile::uuid() const
{
  return FilterTraits<ImportQMMeltpoolTDMSFile>::uuid;
}

std::string ImportQMMeltpoolTDMSFile::humanName() const
{
  return "Import QM Meltpool TDMS File";
}

Parameters ImportQMMeltpoolTDMSFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input TDMS File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ImportQMMeltpoolTDMSFile::clone() const
{
  return std::make_unique<ImportQMMeltpoolTDMSFile>();
}

Result<OutputActions> ImportQMMeltpoolTDMSFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportQMMeltpoolTDMSFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportQMMeltpoolTDMSFile::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
