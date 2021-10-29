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
Result<OutputActions> AverageEdgeFaceCellArrayToVertexArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageVertexArrayPathValue = filterArgs.value<DataPath>(k_AverageVertexArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AverageEdgeFaceCellArrayToVertexArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
