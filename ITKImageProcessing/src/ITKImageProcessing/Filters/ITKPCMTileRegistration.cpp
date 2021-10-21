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
Result<OutputActions> ITKPCMTileRegistration::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pDataContainerPaddingDigitsValue = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKPCMTileRegistrationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ITKPCMTileRegistration::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
