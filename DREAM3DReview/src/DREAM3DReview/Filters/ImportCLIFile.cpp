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
  params.insert(std::make_unique<FileSystemPathParameter>(k_CLIFile_Key, "CLI File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_EdgeDataContainerName_Key, "Edge Data Container", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_LayerIdsArrayName_Key, "Layer Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportCLIFile::clone() const
{
  return std::make_unique<ImportCLIFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportCLIFile::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCLIFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_CLIFile_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_EdgeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pLayerIdsArrayNameValue = filterArgs.value<DataPath>(k_LayerIdsArrayName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportCLIFile::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                    const std::atomic_bool& shouldCancel) const
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
