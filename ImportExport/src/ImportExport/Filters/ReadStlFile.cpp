#include "ReadStlFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadStlFile::name() const
{
  return FilterTraits<ReadStlFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadStlFile::className() const
{
  return FilterTraits<ReadStlFile>::className;
}

//------------------------------------------------------------------------------
Uuid ReadStlFile::uuid() const
{
  return FilterTraits<ReadStlFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadStlFile::humanName() const
{
  return "Import STL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadStlFile::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ReadStlFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilePath_Key, "STL File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_SurfaceMeshDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceNormalsArrayName_Key, "Face Normals", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadStlFile::clone() const
{
  return std::make_unique<ReadStlFile>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ReadStlFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pSurfaceMeshDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceMeshDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReadStlFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ReadStlFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pSurfaceMeshDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceMeshDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
