#include "AlignGeometries.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AlignGeometries::name() const
{
  return FilterTraits<AlignGeometries>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignGeometries::className() const
{
  return FilterTraits<AlignGeometries>::className;
}

//------------------------------------------------------------------------------
Uuid AlignGeometries::uuid() const
{
  return FilterTraits<AlignGeometries>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignGeometries::humanName() const
{
  return "Align Geometries";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignGeometries::defaultTags() const
{
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignGeometries::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_MovingGeometry_Key, "Moving Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TargetGeometry_Key, "Target Geometry", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_AlignmentType_Key, "Alignment Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignGeometries::clone() const
{
  return std::make_unique<AlignGeometries>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignGeometries::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMovingGeometryValue = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  auto pTargetGeometryValue = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  auto pAlignmentTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);

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
  auto action = std::make_unique<AlignGeometriesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> AlignGeometries::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMovingGeometryValue = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  auto pTargetGeometryValue = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  auto pAlignmentTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
