#include "SampleSurfaceMeshSpecifiedPoints.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPoints::name() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPoints>::name.str();
}

//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPoints::className() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPoints>::className;
}

//------------------------------------------------------------------------------
Uuid SampleSurfaceMeshSpecifiedPoints::uuid() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPoints>::uuid;
}

//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPoints::humanName() const
{
  return "Sample Triangle Geometry at Specified Points";
}

//------------------------------------------------------------------------------
std::vector<std::string> SampleSurfaceMeshSpecifiedPoints::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters SampleSurfaceMeshSpecifiedPoints::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Specified Points File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputFilePath_Key, "Sampled Values File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SampleSurfaceMeshSpecifiedPoints::clone() const
{
  return std::make_unique<SampleSurfaceMeshSpecifiedPoints>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SampleSurfaceMeshSpecifiedPoints::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<SampleSurfaceMeshSpecifiedPointsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> SampleSurfaceMeshSpecifiedPoints::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
