#include "EstablishShapeTypes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ShapeTypeSelectionFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EstablishShapeTypes::name() const
{
  return FilterTraits<EstablishShapeTypes>::name.str();
}

//------------------------------------------------------------------------------
std::string EstablishShapeTypes::className() const
{
  return FilterTraits<EstablishShapeTypes>::className;
}

//------------------------------------------------------------------------------
Uuid EstablishShapeTypes::uuid() const
{
  return FilterTraits<EstablishShapeTypes>::uuid;
}

//------------------------------------------------------------------------------
std::string EstablishShapeTypes::humanName() const
{
  return "Establish Shape Types";
}

//------------------------------------------------------------------------------
std::vector<std::string> EstablishShapeTypes::defaultTags() const
{
  return {"#Synthetic Building", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters EstablishShapeTypes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputPhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_ShapeTypesArrayName_Key, "Shape Types", "", DataPath{}));
  /*[x]*/ params.insert(std::make_unique<ShapeTypeSelectionFilterParameter>(k_ShapeTypeData_Key, "Shape Types", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EstablishShapeTypes::clone() const
{
  return std::make_unique<EstablishShapeTypes>();
}

//------------------------------------------------------------------------------
Result<OutputActions> EstablishShapeTypes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pShapeTypesArrayNameValue = filterArgs.value<DataPath>(k_ShapeTypesArrayName_Key);
  auto pShapeTypeDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ShapeTypeData_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<EstablishShapeTypesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> EstablishShapeTypes::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  auto pShapeTypesArrayNameValue = filterArgs.value<DataPath>(k_ShapeTypesArrayName_Key);
  auto pShapeTypeDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ShapeTypeData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
