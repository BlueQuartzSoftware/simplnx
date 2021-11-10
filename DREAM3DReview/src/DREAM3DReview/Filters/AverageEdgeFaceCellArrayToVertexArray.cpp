#include "AverageEdgeFaceCellArrayToVertexArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AverageEdgeFaceCellArrayToVertexArray::name() const
{
  return FilterTraits<AverageEdgeFaceCellArrayToVertexArray>::name.str();
}

//------------------------------------------------------------------------------
std::string AverageEdgeFaceCellArrayToVertexArray::className() const
{
  return FilterTraits<AverageEdgeFaceCellArrayToVertexArray>::className;
}

//------------------------------------------------------------------------------
Uuid AverageEdgeFaceCellArrayToVertexArray::uuid() const
{
  return FilterTraits<AverageEdgeFaceCellArrayToVertexArray>::uuid;
}

//------------------------------------------------------------------------------
std::string AverageEdgeFaceCellArrayToVertexArray::humanName() const
{
  return "Average Edge/Face/Cell Array to Vertex Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> AverageEdgeFaceCellArrayToVertexArray::defaultTags() const
{
  return {"#DREAM3D Review", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters AverageEdgeFaceCellArrayToVertexArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Edge/Face/Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Edge/Face/Cell Array to Average", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_AverageVertexArrayPath_Key, "Average Vertex Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AverageEdgeFaceCellArrayToVertexArray::clone() const
{
  return std::make_unique<AverageEdgeFaceCellArrayToVertexArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AverageEdgeFaceCellArrayToVertexArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageVertexArrayPathValue = filterArgs.value<DataPath>(k_AverageVertexArrayPath_Key);

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
  auto action = std::make_unique<AverageEdgeFaceCellArrayToVertexArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> AverageEdgeFaceCellArrayToVertexArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageVertexArrayPathValue = filterArgs.value<DataPath>(k_AverageVertexArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
