#include "InitializeData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult InitializeData::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
