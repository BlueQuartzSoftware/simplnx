#include "ComputeFeatureRect.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ComputeFeatureRect::name() const
{
  return FilterTraits<ComputeFeatureRect>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureRect::className() const
{
  return FilterTraits<ComputeFeatureRect>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureRect::uuid() const
{
  return FilterTraits<ComputeFeatureRect>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureRect::humanName() const
{
  return "Compute Feature Corners";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureRect::defaultTags() const
{
  return {"#Statistics", "#Reconstruction"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureRect::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureRectArrayPath_Key, "Feature Rect", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureRect::clone() const
{
  return std::make_unique<ComputeFeatureRect>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ComputeFeatureRect::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ComputeFeatureRectAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureRect::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
