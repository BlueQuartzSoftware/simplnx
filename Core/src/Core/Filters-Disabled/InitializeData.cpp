#include "InitializeData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/RangeFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string InitializeData::name() const
{
  return FilterTraits<InitializeData>::name.str();
}

//------------------------------------------------------------------------------
std::string InitializeData::className() const
{
  return FilterTraits<InitializeData>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeData::uuid() const
{
  return FilterTraits<InitializeData>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeData::humanName() const
{
  return "Initialize Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeData::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters InitializeData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CellAttributeMatrixPaths_Key, "Cell Arrays", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<Int32Parameter>(k_XMin_Key, "X Min (Column)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YMin_Key, "Y Min (Row)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZMin_Key, "Z Min (Plane)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_XMax_Key, "X Max (Column)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YMax_Key, "Y Max (Row)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZMax_Key, "Z Max (Plane)", "", 1234356));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "", 0, ChoicesParameter::Choices{"Manual", "Random", "Random With Range"}));
  params.insert(std::make_unique<Float64Parameter>(k_InitValue_Key, "Initialization Value", "", 2.3456789));
  /*[x]*/ params.insert(std::make_unique<RangeFilterParameter>(k_InitRange_Key, "Initialization Range", "", {}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_InitType_Key, k_InitValue_Key, 0);
  params.linkParameters(k_InitType_Key, k_InitRange_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeData::clone() const
{
  return std::make_unique<InitializeData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCellAttributeMatrixPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CellAttributeMatrixPaths_Key);
  auto pXMinValue = filterArgs.value<int32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<int32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<int32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<int32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<int32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<int32>(k_ZMax_Key);
  auto pInitTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InitType_Key);
  auto pInitValue = filterArgs.value<float64>(k_InitValue_Key);
  auto pInitRangeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitRange_Key);

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
  auto action = std::make_unique<InitializeDataAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> InitializeData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCellAttributeMatrixPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CellAttributeMatrixPaths_Key);
  auto pXMinValue = filterArgs.value<int32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<int32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<int32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<int32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<int32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<int32>(k_ZMax_Key);
  auto pInitTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InitType_Key);
  auto pInitValue = filterArgs.value<float64>(k_InitValue_Key);
  auto pInitRangeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitRange_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
