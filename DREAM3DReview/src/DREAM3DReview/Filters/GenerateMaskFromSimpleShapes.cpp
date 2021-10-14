#include "GenerateMaskFromSimpleShapes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
std::string GenerateMaskFromSimpleShapes::name() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::name.str();
}

std::string GenerateMaskFromSimpleShapes::className() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::className;
}

Uuid GenerateMaskFromSimpleShapes::uuid() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::uuid;
}

std::string GenerateMaskFromSimpleShapes::humanName() const
{
  return "Generate Mask From Simple Shapes";
}

Parameters GenerateMaskFromSimpleShapes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_MaskShape_Key, "Mask Shape", "", 0, ChoicesParameter::Choices{"Ellipsoid", "Box", "Cylinder"}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentersArrayPath_Key, "Centers", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AxesLengthArrayPath_Key, "Ellipsoid Axes Lengths", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoxDimensionsArrayPath_Key, "Box Dimensions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CylinderRadiusArrayPath_Key, "Cylinder Radii", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CylinderHeightArrayPath_Key, "Cylinder Heights", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_MaskShape_Key, k_AxesLengthArrayPath_Key, 0);
  params.linkParameters(k_MaskShape_Key, k_BoxDimensionsArrayPath_Key, 1);
  params.linkParameters(k_MaskShape_Key, k_CylinderHeightArrayPath_Key, 2);
  params.linkParameters(k_MaskShape_Key, k_CylinderRadiusArrayPath_Key, 3);

  return params;
}

IFilter::UniquePointer GenerateMaskFromSimpleShapes::clone() const
{
  return std::make_unique<GenerateMaskFromSimpleShapes>();
}

Result<OutputActions> GenerateMaskFromSimpleShapes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMaskShapeValue = filterArgs.value<ChoicesParameter::ValueType>(k_MaskShape_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCentersArrayPathValue = filterArgs.value<DataPath>(k_CentersArrayPath_Key);
  auto pAxesLengthArrayPathValue = filterArgs.value<DataPath>(k_AxesLengthArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pCylinderRadiusArrayPathValue = filterArgs.value<DataPath>(k_CylinderRadiusArrayPath_Key);
  auto pCylinderHeightArrayPathValue = filterArgs.value<DataPath>(k_CylinderHeightArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateMaskFromSimpleShapesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> GenerateMaskFromSimpleShapes::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaskShapeValue = filterArgs.value<ChoicesParameter::ValueType>(k_MaskShape_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCentersArrayPathValue = filterArgs.value<DataPath>(k_CentersArrayPath_Key);
  auto pAxesLengthArrayPathValue = filterArgs.value<DataPath>(k_AxesLengthArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pCylinderRadiusArrayPathValue = filterArgs.value<DataPath>(k_CylinderRadiusArrayPath_Key);
  auto pCylinderHeightArrayPathValue = filterArgs.value<DataPath>(k_CylinderHeightArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
