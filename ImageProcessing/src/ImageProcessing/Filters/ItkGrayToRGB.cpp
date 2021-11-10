#include "ItkGrayToRGB.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkGrayToRGB::name() const
{
  return FilterTraits<ItkGrayToRGB>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkGrayToRGB::className() const
{
  return FilterTraits<ItkGrayToRGB>::className;
}

//------------------------------------------------------------------------------
Uuid ItkGrayToRGB::uuid() const
{
  return FilterTraits<ItkGrayToRGB>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkGrayToRGB::humanName() const
{
  return "Convert Grayscale to RGB (Merge Channels) (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkGrayToRGB::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkGrayToRGB::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RedArrayPath_Key, "Red Channel", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GreenArrayPath_Key, "Green Channel", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BlueArrayPath_Key, "Blue Channel", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "RGB Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkGrayToRGB::clone() const
{
  return std::make_unique<ItkGrayToRGB>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkGrayToRGB::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRedArrayPathValue = filterArgs.value<DataPath>(k_RedArrayPath_Key);
  auto pGreenArrayPathValue = filterArgs.value<DataPath>(k_GreenArrayPath_Key);
  auto pBlueArrayPathValue = filterArgs.value<DataPath>(k_BlueArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

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
  auto action = std::make_unique<ItkGrayToRGBAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ItkGrayToRGB::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRedArrayPathValue = filterArgs.value<DataPath>(k_RedArrayPath_Key);
  auto pGreenArrayPathValue = filterArgs.value<DataPath>(k_GreenArrayPath_Key);
  auto pBlueArrayPathValue = filterArgs.value<DataPath>(k_BlueArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
