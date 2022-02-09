#include "GenerateMaskFromSimpleShapes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateMaskFromSimpleShapes::name() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateMaskFromSimpleShapes::className() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateMaskFromSimpleShapes::uuid() const
{
  return FilterTraits<GenerateMaskFromSimpleShapes>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateMaskFromSimpleShapes::humanName() const
{
  return "Generate Mask From Simple Shapes";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateMaskFromSimpleShapes::defaultTags() const
{
  return {"#Unsupported", "#DREAM3DReview"};
}

//------------------------------------------------------------------------------
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
  params.linkParameters(k_MaskShape_Key, k_CylinderRadiusArrayPath_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateMaskFromSimpleShapes::clone() const
{
  return std::make_unique<GenerateMaskFromSimpleShapes>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateMaskFromSimpleShapes::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMaskShapeValue = filterArgs.value<ChoicesParameter::ValueType>(k_MaskShape_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCentersArrayPathValue = filterArgs.value<DataPath>(k_CentersArrayPath_Key);
  auto pAxesLengthArrayPathValue = filterArgs.value<DataPath>(k_AxesLengthArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pCylinderRadiusArrayPathValue = filterArgs.value<DataPath>(k_CylinderRadiusArrayPath_Key);
  auto pCylinderHeightArrayPathValue = filterArgs.value<DataPath>(k_CylinderHeightArrayPath_Key);

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
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pMaskArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateMaskFromSimpleShapes::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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
