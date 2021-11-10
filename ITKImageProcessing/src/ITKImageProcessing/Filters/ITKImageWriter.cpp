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
//------------------------------------------------------------------------------
std::string ITKImageWriter::name() const
{
  return FilterTraits<ITKImageWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKImageWriter::className() const
{
  return FilterTraits<ITKImageWriter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageWriter::uuid() const
{
  return FilterTraits<ITKImageWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageWriter::humanName() const
{
  return "ITK::Image Export";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageWriter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageWriter::clone() const
{
  return std::make_unique<ITKImageWriter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto pIndexOffsetValue = filterArgs.value<int32>(k_IndexOffset_Key);
  auto pImageArrayPathValue = filterArgs.value<DataPath>(k_ImageArrayPath_Key);

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
  auto action = std::make_unique<ITKImageWriterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKImageWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
