#include "WarpRegularGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FourthOrderPolynomialFilterParameter.hpp"
#include "complex/Parameters/SecondOrderPolynomialFilterParameter.hpp"
#include "complex/Parameters/ThirdOrderPolynomialFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WarpRegularGrid::name() const
{
  return FilterTraits<WarpRegularGrid>::name.str();
}

//------------------------------------------------------------------------------
std::string WarpRegularGrid::className() const
{
  return FilterTraits<WarpRegularGrid>::className;
}

//------------------------------------------------------------------------------
Uuid WarpRegularGrid::uuid() const
{
  return FilterTraits<WarpRegularGrid>::uuid;
}

//------------------------------------------------------------------------------
std::string WarpRegularGrid::humanName() const
{
  return "Warp Rectilinear Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> WarpRegularGrid::defaultTags() const
{
  return {"#Sampling", "#Warping"};
}

//------------------------------------------------------------------------------
Parameters WarpRegularGrid::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PolyOrder_Key, "Polynomial Order", "", 0, ChoicesParameter::Choices{"2nd", "3rd", "4th"}));
  /*[x]*/ params.insert(std::make_unique<SecondOrderPolynomialFilterParameter>(k_SecondOrderACoeff_Key, "Second Order A Coefficients", "", {}));
  /*[x]*/ params.insert(std::make_unique<SecondOrderPolynomialFilterParameter>(k_SecondOrderBCoeff_Key, "Second Order B Coefficients", "", {}));
  /*[x]*/ params.insert(std::make_unique<ThirdOrderPolynomialFilterParameter>(k_ThirdOrderACoeff_Key, "Third Order A Coefficients", "", {}));
  /*[x]*/ params.insert(std::make_unique<ThirdOrderPolynomialFilterParameter>(k_ThirdOrderBCoeff_Key, "Third Order B Coefficients", "", {}));
  /*[x]*/ params.insert(std::make_unique<FourthOrderPolynomialFilterParameter>(k_FourthOrderACoeff_Key, "Fourth Order A Coefficients", "", {}));
  /*[x]*/ params.insert(std::make_unique<FourthOrderPolynomialFilterParameter>(k_FourthOrderBCoeff_Key, "Fourth Order B Coefficients", "", {}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewDataContainer_Key, "Save as New Data Container", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PolyOrder_Key, k_SecondOrderACoeff_Key, 0);
  params.linkParameters(k_PolyOrder_Key, k_ThirdOrderACoeff_Key, 1);
  params.linkParameters(k_PolyOrder_Key, k_FourthOrderACoeff_Key, 2);
  params.linkParameters(k_PolyOrder_Key, k_SecondOrderBCoeff_Key, 0);
  params.linkParameters(k_PolyOrder_Key, k_ThirdOrderBCoeff_Key, 1);
  params.linkParameters(k_PolyOrder_Key, k_FourthOrderBCoeff_Key, 2);
  params.linkParameters(k_SaveAsNewDataContainer_Key, k_NewDataContainerName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WarpRegularGrid::clone() const
{
  return std::make_unique<WarpRegularGrid>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WarpRegularGrid::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPolyOrderValue = filterArgs.value<ChoicesParameter::ValueType>(k_PolyOrder_Key);
  auto pSecondOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderACoeff_Key);
  auto pSecondOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderBCoeff_Key);
  auto pThirdOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderACoeff_Key);
  auto pThirdOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderBCoeff_Key);
  auto pFourthOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderACoeff_Key);
  auto pFourthOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderBCoeff_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

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
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WarpRegularGrid::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPolyOrderValue = filterArgs.value<ChoicesParameter::ValueType>(k_PolyOrder_Key);
  auto pSecondOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderACoeff_Key);
  auto pSecondOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderBCoeff_Key);
  auto pThirdOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderACoeff_Key);
  auto pThirdOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderBCoeff_Key);
  auto pFourthOrderACoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderACoeff_Key);
  auto pFourthOrderBCoeffValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderBCoeff_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
