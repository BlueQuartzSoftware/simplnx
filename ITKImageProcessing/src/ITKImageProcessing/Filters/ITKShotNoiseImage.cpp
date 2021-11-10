#include "ITKShotNoiseImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::name() const
{
  return FilterTraits<ITKShotNoiseImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::className() const
{
  return FilterTraits<ITKShotNoiseImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKShotNoiseImage::uuid() const
{
  return FilterTraits<ITKShotNoiseImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::humanName() const
{
  return "ITK::Shot Noise Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKShotNoiseImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK ImageNoise"};
}

//------------------------------------------------------------------------------
Parameters ITKShotNoiseImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Scale_Key, "Scale", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Seed_Key, "Seed", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKShotNoiseImage::clone() const
{
  return std::make_unique<ITKShotNoiseImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKShotNoiseImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pScaleValue = filterArgs.value<float64>(k_Scale_Key);
  auto pSeedValue = filterArgs.value<float64>(k_Seed_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

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
  auto action = std::make_unique<ITKShotNoiseImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKShotNoiseImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScaleValue = filterArgs.value<float64>(k_Scale_Key);
  auto pSeedValue = filterArgs.value<float64>(k_Seed_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
