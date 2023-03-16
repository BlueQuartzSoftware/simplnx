#include "WriteStlFileFilter.hpp"

#include "ComplexCore/Filters/Algorithms/WriteStlFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteStlFileFilter::name() const
{
  return FilterTraits<WriteStlFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::className() const
{
  return FilterTraits<WriteStlFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteStlFileFilter::uuid() const
{
  return FilterTraits<WriteStlFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::humanName() const
{
  return "Export STL Files from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStlFileFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteStlFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{""});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputStlDirectory_Key, "Output STL Directory", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputStlPrefix_Key, "STL File Prefix", "", "SomeString"));
  
  params.insertSeparator(Parameters::Separator{"Required Face Data Array"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{},
                                                          complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteStlFileFilter::clone() const
{
  return std::make_unique<WriteStlFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteStlFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto pOutputStlDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  auto pOutputStlPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteStlFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  WriteStlFileInputValues inputValues;

  inputValues.OutputStlDirectory = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  inputValues.OutputStlPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  return WriteStlFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
