#include "ReadStlFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult ReadStlFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pSurfaceMeshDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceMeshDataContainerName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
  // to the DataStructure. If none are available then create a new custom Action subclass.
  // If your filter does not make any structural modifications to the DataStructure then
  // you can skip this code.
  auto outputAction = std::make_unique<EmptyAction>();

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable past this point.
  resultOutputActions.value().actions.push_back(std::move(outputAction));

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
