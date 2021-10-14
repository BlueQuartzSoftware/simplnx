#include "ITKMinMaxCurvatureFlowImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKMinMaxCurvatureFlowImage::name() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImage>::name.str();
}

Uuid ITKMinMaxCurvatureFlowImage::uuid() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImage>::uuid;
}

std::string ITKMinMaxCurvatureFlowImage::humanName() const
{
  return "ITK::Min Max Curvature Flow Image Filter";
}

Parameters ITKMinMaxCurvatureFlowImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_StencilRadius_Key, "StencilRadius", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKMinMaxCurvatureFlowImage::clone() const
{
  return std::make_unique<ITKMinMaxCurvatureFlowImage>();
}

Result<OutputActions> ITKMinMaxCurvatureFlowImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pTimeStepValue = filterArgs.value<float64>(k_TimeStep_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pStencilRadiusValue = filterArgs.value<int32>(k_StencilRadius_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKMinMaxCurvatureFlowImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKMinMaxCurvatureFlowImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pTimeStepValue = filterArgs.value<float64>(k_TimeStep_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pStencilRadiusValue = filterArgs.value<int32>(k_StencilRadius_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
