#include "ExtractAttributeArraysFromGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractAttributeArraysFromGeometry::name() const
{
  return FilterTraits<ExtractAttributeArraysFromGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractAttributeArraysFromGeometry::className() const
{
  return FilterTraits<ExtractAttributeArraysFromGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractAttributeArraysFromGeometry::uuid() const
{
  return FilterTraits<ExtractAttributeArraysFromGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractAttributeArraysFromGeometry::humanName() const
{
  return "Extract Attribute Arrays from Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractAttributeArraysFromGeometry::defaultTags() const
{
  return {"#Core", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters ExtractAttributeArraysFromGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_XBoundsArrayPath_Key, "X Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_YBoundsArrayPath_Key, "Y Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ZBoundsArrayPath_Key, "Z Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath0_Key, "Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath1_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedEdgeListArrayPath_Key, "Edge List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath2_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedTriListArrayPath_Key, "Triangle List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath3_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedQuadListArrayPath_Key, "Quadrilateral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath4_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedTetListArrayPath_Key, "Tetrahedral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath5_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedHexListArrayPath_Key, "Hexahedral List", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractAttributeArraysFromGeometry::clone() const
{
  return std::make_unique<ExtractAttributeArraysFromGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractAttributeArraysFromGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);

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

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractAttributeArraysFromGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
