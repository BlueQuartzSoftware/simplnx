#include "EbsdToH5Ebsd.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/EbsdToH5EbsdFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EbsdToH5Ebsd::name() const
{
  return FilterTraits<EbsdToH5Ebsd>::name.str();
}

//------------------------------------------------------------------------------
std::string EbsdToH5Ebsd::className() const
{
  return FilterTraits<EbsdToH5Ebsd>::className;
}

//------------------------------------------------------------------------------
Uuid EbsdToH5Ebsd::uuid() const
{
  return FilterTraits<EbsdToH5Ebsd>::uuid;
}

//------------------------------------------------------------------------------
std::string EbsdToH5Ebsd::humanName() const
{
  return "Import Orientation File(s) to H5EBSD";
}

//------------------------------------------------------------------------------
std::vector<std::string> EbsdToH5Ebsd::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters EbsdToH5Ebsd::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<EbsdToH5EbsdFilterParameter>(k_OrientationData_Key, "Import Orientation Data", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EbsdToH5Ebsd::clone() const
{
  return std::make_unique<EbsdToH5Ebsd>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EbsdToH5Ebsd::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOrientationDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OrientationData_Key);

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
  auto action = std::make_unique<EbsdToH5EbsdAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> EbsdToH5Ebsd::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOrientationDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OrientationData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
