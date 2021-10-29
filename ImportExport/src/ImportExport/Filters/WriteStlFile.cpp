#include "WriteStlFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteStlFile::name() const
{
  return FilterTraits<WriteStlFile>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteStlFile::className() const
{
  return FilterTraits<WriteStlFile>::className;
}

//------------------------------------------------------------------------------
Uuid WriteStlFile::uuid() const
{
  return FilterTraits<WriteStlFile>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteStlFile::humanName() const
{
  return "Export STL Files from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStlFile::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters WriteStlFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputStlDirectory_Key, "Output STL Directory", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputStlPrefix_Key, "STL File Prefix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteStlFile::clone() const
{
  return std::make_unique<WriteStlFile>();
}

//------------------------------------------------------------------------------
Result<OutputActions> WriteStlFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputStlDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  auto pOutputStlPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WriteStlFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> WriteStlFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputStlDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  auto pOutputStlPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
