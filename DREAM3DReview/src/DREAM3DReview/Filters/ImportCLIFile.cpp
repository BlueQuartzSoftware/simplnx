#include "ImportCLIFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportCLIFile::name() const
{
  return FilterTraits<ImportCLIFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportCLIFile::className() const
{
  return FilterTraits<ImportCLIFile>::className;
}

//------------------------------------------------------------------------------
Uuid ImportCLIFile::uuid() const
{
  return FilterTraits<ImportCLIFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportCLIFile::humanName() const
{
  return "Import CLI File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportCLIFile::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportCLIFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_CLIFile_Key, "CLI File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_EdgeDataContainerName_Key, "Edge Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_LayerIdsArrayName_Key, "Layer Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportCLIFile::clone() const
{
  return std::make_unique<ImportCLIFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportCLIFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCLIFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CLIFile_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pLayerIdsArrayNameValue = filterArgs.value<DataPath>(k_LayerIdsArrayName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportCLIFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ImportCLIFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCLIFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CLIFile_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pLayerIdsArrayNameValue = filterArgs.value<DataPath>(k_LayerIdsArrayName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
