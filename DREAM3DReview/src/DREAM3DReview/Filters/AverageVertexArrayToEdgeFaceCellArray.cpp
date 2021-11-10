#include "AverageVertexArrayToEdgeFaceCellArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AverageVertexArrayToEdgeFaceCellArray::name() const
{
  return FilterTraits<AverageVertexArrayToEdgeFaceCellArray>::name.str();
}

//------------------------------------------------------------------------------
std::string AverageVertexArrayToEdgeFaceCellArray::className() const
{
  return FilterTraits<AverageVertexArrayToEdgeFaceCellArray>::className;
}

//------------------------------------------------------------------------------
Uuid AverageVertexArrayToEdgeFaceCellArray::uuid() const
{
  return FilterTraits<AverageVertexArrayToEdgeFaceCellArray>::uuid;
}

//------------------------------------------------------------------------------
std::string AverageVertexArrayToEdgeFaceCellArray::humanName() const
{
  return "Average Vertex Array to Edge/Face/Cell Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> AverageVertexArrayToEdgeFaceCellArray::defaultTags() const
{
  return {"#DREAM3D Review", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters AverageVertexArrayToEdgeFaceCellArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_WeightedAverage_Key, "Perform Weighted Average", "", false));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Vertex Array to Average", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge/Face/Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_AverageCellArrayPath_Key, "Average Edge/Face/Cell Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AverageVertexArrayToEdgeFaceCellArray::clone() const
{
  return std::make_unique<AverageVertexArrayToEdgeFaceCellArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AverageVertexArrayToEdgeFaceCellArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pWeightedAverageValue = filterArgs.value<bool>(k_WeightedAverage_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageCellArrayPathValue = filterArgs.value<DataPath>(k_AverageCellArrayPath_Key);

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
  auto action = std::make_unique<AverageVertexArrayToEdgeFaceCellArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> AverageVertexArrayToEdgeFaceCellArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWeightedAverageValue = filterArgs.value<bool>(k_WeightedAverage_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageCellArrayPathValue = filterArgs.value<DataPath>(k_AverageCellArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
