#include "FindMinkowskiBouligandDimension.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::name() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::name.str();
}

//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::className() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::className;
}

//------------------------------------------------------------------------------
Uuid FindMinkowskiBouligandDimension::uuid() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::uuid;
}

//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::humanName() const
{
  return "Find Minkowski-Bouligand Dimension";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindMinkowskiBouligandDimension::defaultTags() const
{
  return {"#Statistics", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters FindMinkowskiBouligandDimension::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AttributeMatrixName_Key, "Fractal Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MinkowskiBouligandDimensionArrayName_Key, "Minkowski-Bouligand Dimension", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindMinkowskiBouligandDimension::clone() const
{
  return std::make_unique<FindMinkowskiBouligandDimension>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindMinkowskiBouligandDimension::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pMinkowskiBouligandDimensionArrayNameValue = filterArgs.value<DataPath>(k_MinkowskiBouligandDimensionArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindMinkowskiBouligandDimensionAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindMinkowskiBouligandDimension::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pMinkowskiBouligandDimensionArrayNameValue = filterArgs.value<DataPath>(k_MinkowskiBouligandDimensionArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
