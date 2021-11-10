#include "FindSizes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSizes::name() const
{
  return FilterTraits<FindSizes>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSizes::className() const
{
  return FilterTraits<FindSizes>::className;
}

//------------------------------------------------------------------------------
Uuid FindSizes::uuid() const
{
  return FilterTraits<FindSizes>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSizes::humanName() const
{
  return "Find Feature Sizes";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSizes::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindSizes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Save Element Sizes", "", false));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EquivalentDiametersArrayName_Key, "Equivalent Diameters", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumElementsArrayName_Key, "Number of Elements", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesArrayName_Key, "Volumes", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSizes::clone() const
{
  return std::make_unique<FindSizes>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSizes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSaveElementSizesValue = filterArgs.value<bool>(k_SaveElementSizes_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pEquivalentDiametersArrayNameValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayName_Key);
  auto pNumElementsArrayNameValue = filterArgs.value<DataPath>(k_NumElementsArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

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
  auto action = std::make_unique<FindSizesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindSizes::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSaveElementSizesValue = filterArgs.value<bool>(k_SaveElementSizes_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pEquivalentDiametersArrayNameValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayName_Key);
  auto pNumElementsArrayNameValue = filterArgs.value<DataPath>(k_NumElementsArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
