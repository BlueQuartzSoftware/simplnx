#include "ImportPrintRiteHDF5File.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ImportPrintRiteHDF5File::name() const
{
  return FilterTraits<ImportPrintRiteHDF5File>::name.str();
}

Uuid ImportPrintRiteHDF5File::uuid() const
{
  return FilterTraits<ImportPrintRiteHDF5File>::uuid;
}

std::string ImportPrintRiteHDF5File::humanName() const
{
  return "Import PrintRite HDF5 File";
}

Parameters ImportPrintRiteHDF5File::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input TDMS-HDF5 File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_HFDataContainerName_Key, "High Frequency Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_HFDataName_Key, "High Frequency Vertex Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_HFSliceIdsArrayName_Key, "High Frequency Slice Ids", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Feature Data"});
  params.insert(std::make_unique<StringParameter>(k_HFSliceDataName_Key, "High Frequency Slice Attribute Matrix", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ImportPrintRiteHDF5File::clone() const
{
  return std::make_unique<ImportPrintRiteHDF5File>();
}

Result<OutputActions> ImportPrintRiteHDF5File::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pHFDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataContainerName_Key);
  auto pHFDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataName_Key);
  auto pHFSliceIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceIdsArrayName_Key);
  auto pHFSliceDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceDataName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportPrintRiteHDF5FileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportPrintRiteHDF5File::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pHFDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataContainerName_Key);
  auto pHFDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFDataName_Key);
  auto pHFSliceIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceIdsArrayName_Key);
  auto pHFSliceDataNameValue = filterArgs.value<StringParameter::ValueType>(k_HFSliceDataName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
