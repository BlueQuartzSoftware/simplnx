#include "ITKPCMTileRegistration.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKPCMTileRegistration::name() const
{
  return FilterTraits<ITKPCMTileRegistration>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKPCMTileRegistration::className() const
{
  return FilterTraits<ITKPCMTileRegistration>::className;
}

//------------------------------------------------------------------------------
Uuid ITKPCMTileRegistration::uuid() const
{
  return FilterTraits<ITKPCMTileRegistration>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKPCMTileRegistration::humanName() const
{
  return "ITK::Compute Tile Transformations (PCM Method)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKPCMTileRegistration::defaultTags() const
{
  return {"#IO", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters ITKPCMTileRegistration::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<Int32Parameter>(k_DataContainerPaddingDigits_Key, "Padding Digits for DataContainer Names", "", 1234356));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPrefix_Key, "Data Container Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CommonAttributeMatrixName_Key, "Common Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CommonDataArrayName_Key, "Common Data Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKPCMTileRegistration::clone() const
{
  return std::make_unique<ITKPCMTileRegistration>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKPCMTileRegistration::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pDataContainerPaddingDigitsValue = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

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
  auto action = std::make_unique<ITKPCMTileRegistrationAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKPCMTileRegistration::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pDataContainerPaddingDigitsValue = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
