#include "WarpRegularGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
  params.linkParameters(k_PolyOrder_Key, k_SecondOrderBCoeff_Key, 3);
  params.linkParameters(k_PolyOrder_Key, k_ThirdOrderBCoeff_Key, 4);
  params.linkParameters(k_PolyOrder_Key, k_FourthOrderBCoeff_Key, 5);
  params.linkParameters(k_SaveAsNewDataContainer_Key, k_NewDataContainerName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WarpRegularGrid::clone() const
{
  return std::make_unique<WarpRegularGrid>();
}

//------------------------------------------------------------------------------
Result<OutputActions> WarpRegularGrid::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WarpRegularGridAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> WarpRegularGrid::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
