#include "LabelTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LabelTriangleGeometry::name() const
{
  return FilterTraits<LabelTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string LabelTriangleGeometry::className() const
{
  return FilterTraits<LabelTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid LabelTriangleGeometry::uuid() const
{
  return FilterTraits<LabelTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string LabelTriangleGeometry::humanName() const
{
  return "Label CAD Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> LabelTriangleGeometry::defaultTags() const
{
  return {"#Reconstruction", "#Grouping"};
}

//------------------------------------------------------------------------------
Parameters LabelTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CADDataContainerName_Key, "CAD Geometry", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_TriangleAttributeMatrixName_Key, "Triangle Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_RegionIdArrayName_Key, "Region Ids", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LabelTriangleGeometry::clone() const
{
  return std::make_unique<LabelTriangleGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LabelTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pTriangleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  auto pRegionIdArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RegionIdArrayName_Key);

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
  auto action = std::make_unique<LabelTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> LabelTriangleGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pTriangleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  auto pRegionIdArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RegionIdArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
