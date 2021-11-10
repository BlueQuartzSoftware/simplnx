#include "CombineStlFiles.hpp"

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
std::string CombineStlFiles::name() const
{
  return FilterTraits<CombineStlFiles>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineStlFiles::className() const
{
  return FilterTraits<CombineStlFiles>::className;
}

//------------------------------------------------------------------------------
Uuid CombineStlFiles::uuid() const
{
  return FilterTraits<CombineStlFiles>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineStlFiles::humanName() const
{
  return "Combine STL Files";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineStlFiles::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters CombineStlFiles::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilesPath_Key, "Path to STL Files", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputDir));
  params.insert(std::make_unique<StringParameter>(k_TriangleDataContainerName_Key, "Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceNormalsArrayName_Key, "Face Normals", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineStlFiles::clone() const
{
  return std::make_unique<CombineStlFiles>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineStlFiles::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStlFilesPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilesPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

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
  auto action = std::make_unique<CombineStlFilesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> CombineStlFiles::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStlFilesPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilesPath_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
