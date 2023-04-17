#include "SampleSurfaceMeshSpecifiedPointsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/SampleSurfaceMeshSpecifiedPoints.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPointsFilter::name() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPointsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPointsFilter::className() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPointsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SampleSurfaceMeshSpecifiedPointsFilter::uuid() const
{
  return FilterTraits<SampleSurfaceMeshSpecifiedPointsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SampleSurfaceMeshSpecifiedPointsFilter::humanName() const
{
  return "Sample Triangle Geometry at Specified Points";
}

//------------------------------------------------------------------------------
std::vector<std::string> SampleSurfaceMeshSpecifiedPointsFilter::defaultTags() const
{
  return {"Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters SampleSurfaceMeshSpecifiedPointsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Specified Points File", "", fs::path("DefaultInputFileName"), FileSystemPathParameter::ExtensionsType{"*.*"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFilePath_Key, "Sampled Values File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SampleSurfaceMeshSpecifiedPointsFilter::clone() const
{
  return std::make_unique<SampleSurfaceMeshSpecifiedPointsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SampleSurfaceMeshSpecifiedPointsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                               const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pOutputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SampleSurfaceMeshSpecifiedPointsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  SampleSurfaceMeshSpecifiedPointsInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  inputValues.OutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);

  return SampleSurfaceMeshSpecifiedPoints(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
