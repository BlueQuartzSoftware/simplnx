#include "ITKSignedMaurerDistanceMapImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::name() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::className() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSignedMaurerDistanceMapImage::uuid() const
{
  return FilterTraits<ITKSignedMaurerDistanceMapImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSignedMaurerDistanceMapImage::humanName() const
{
  return "ITK::Signed Maurer Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSignedMaurerDistanceMapImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKSignedMaurerDistanceMapImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "InsideIsPositive", "", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "SquaredDistance", "", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSignedMaurerDistanceMapImage::clone() const
{
  return std::make_unique<ITKSignedMaurerDistanceMapImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSignedMaurerDistanceMapImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInsideIsPositiveValue = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto pSquaredDistanceValue = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto pBackgroundValueValue = filterArgs.value<float64>(k_BackgroundValue_Key);
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
  auto action = std::make_unique<ITKSignedMaurerDistanceMapImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKSignedMaurerDistanceMapImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInsideIsPositiveValue = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto pSquaredDistanceValue = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto pBackgroundValueValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
