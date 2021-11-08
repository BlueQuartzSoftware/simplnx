#include "WriteTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteTriangleGeometry::name() const
{
  return FilterTraits<WriteTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteTriangleGeometry::className() const
{
  return FilterTraits<WriteTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid WriteTriangleGeometry::uuid() const
{
  return FilterTraits<WriteTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteTriangleGeometry::humanName() const
{
  return "Export Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteTriangleGeometry::defaultTags() const
{
  return {"#Core", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters WriteTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputNodesFile_Key, "Output Nodes File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputTrianglesFile_Key, "Output Triangles File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerSelection_Key, "DataContainer", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteTriangleGeometry::clone() const
{
  return std::make_unique<WriteTriangleGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputNodesFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputNodesFile_Key);
  auto pOutputTrianglesFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputTrianglesFile_Key);
  auto pDataContainerSelectionValue = filterArgs.value<DataPath>(k_DataContainerSelection_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WriteTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> WriteTriangleGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputNodesFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputNodesFile_Key);
  auto pOutputTrianglesFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputTrianglesFile_Key);
  auto pDataContainerSelectionValue = filterArgs.value<DataPath>(k_DataContainerSelection_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
