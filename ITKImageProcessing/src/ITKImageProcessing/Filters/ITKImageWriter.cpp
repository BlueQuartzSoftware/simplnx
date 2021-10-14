#include "ITKImageWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ITKImageWriter::name() const
{
  return FilterTraits<ITKImageWriter>::name.str();
}

std::string ITKImageWriter::className() const
{
  return FilterTraits<ITKImageWriter>::className;
}

Uuid ITKImageWriter::uuid() const
{
  return FilterTraits<ITKImageWriter>::uuid;
}

std::string ITKImageWriter::humanName() const
{
  return "ITK::Image Export";
}

Parameters ITKImageWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<Int32Parameter>(k_IndexOffset_Key, "Index Offset", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Image Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageArrayPath_Key, "Image", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ITKImageWriter::clone() const
{
  return std::make_unique<ITKImageWriter>();
}

Result<OutputActions> ITKImageWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto pIndexOffsetValue = filterArgs.value<int32>(k_IndexOffset_Key);
  auto pImageArrayPathValue = filterArgs.value<DataPath>(k_ImageArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKImageWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKImageWriter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto pIndexOffsetValue = filterArgs.value<int32>(k_IndexOffset_Key);
  auto pImageArrayPathValue = filterArgs.value<DataPath>(k_ImageArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
