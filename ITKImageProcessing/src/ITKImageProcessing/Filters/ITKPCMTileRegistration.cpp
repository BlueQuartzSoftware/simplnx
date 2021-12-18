#include "ITKPCMTileRegistration.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkPCMTileRegistrationFilter.h>

namespace
{
struct ITKPCMTileRegistrationFilterCreationFunctor
{
  VectorInt32Parameter::ValueType m_ColumnMontageLimits;
  VectorInt32Parameter::ValueType m_RowMontageLimits;
  int32 m_DataContainerPaddingDigits;
  StringParameter::ValueType m_DataContainerPrefix;
  StringParameter::ValueType m_CommonAttributeMatrixName;
  StringParameter::ValueType m_CommonDataArrayName;
};
} // namespace

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
IFilter::PreflightResult ITKPCMTileRegistration::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pDataContainerPaddingDigits = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  auto pDataContainerPrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pCommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;
  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  resultOutputActions = ITK::DataCheck(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath);

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
Result<> ITKPCMTileRegistration::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pDataContainerPaddingDigits = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  auto pDataContainerPrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pCommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKPCMTileRegistrationFilterCreationFunctor itkFunctor;
  itkFunctor.m_ColumnMontageLimits = pColumnMontageLimits;
  itkFunctor.m_RowMontageLimits = pRowMontageLimits;
  itkFunctor.m_DataContainerPaddingDigits = pDataContainerPaddingDigits;
  itkFunctor.m_DataContainerPrefix = pDataContainerPrefix;
  itkFunctor.m_CommonAttributeMatrixName = pCommonAttributeMatrixName;
  itkFunctor.m_CommonDataArrayName = pCommonDataArrayName;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
