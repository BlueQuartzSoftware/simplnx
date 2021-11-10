#include "CropVertexGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CropVertexGeometry::name() const
{
  return FilterTraits<CropVertexGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string CropVertexGeometry::className() const
{
  return FilterTraits<CropVertexGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CropVertexGeometry::uuid() const
{
  return FilterTraits<CropVertexGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CropVertexGeometry::humanName() const
{
  return "Crop Geometry (Vertex)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropVertexGeometry::defaultTags() const
{
  return {"#Core", "#Croping Cutting"};
}

//------------------------------------------------------------------------------
Parameters CropVertexGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Vertex Geometry to Crop", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_XMin_Key, "X Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_YMin_Key, "Y Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ZMin_Key, "Z Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_XMax_Key, "X Max", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_YMax_Key, "Y Max", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ZMax_Key, "Z Max", "", 1.23345f));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CroppedDataContainerName_Key, "Cropped Data Container", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropVertexGeometry::clone() const
{
  return std::make_unique<CropVertexGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropVertexGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXMinValue = filterArgs.value<float32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<float32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<float32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<float32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<float32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<float32>(k_ZMax_Key);
  auto pCroppedDataContainerNameValue = filterArgs.value<DataPath>(k_CroppedDataContainerName_Key);

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
  auto action = std::make_unique<CropVertexGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> CropVertexGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXMinValue = filterArgs.value<float32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<float32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<float32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<float32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<float32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<float32>(k_ZMax_Key);
  auto pCroppedDataContainerNameValue = filterArgs.value<DataPath>(k_CroppedDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
