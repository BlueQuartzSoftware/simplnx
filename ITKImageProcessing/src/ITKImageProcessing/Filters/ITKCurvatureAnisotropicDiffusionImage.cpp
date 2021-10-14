#include "ITKCurvatureAnisotropicDiffusionImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKCurvatureAnisotropicDiffusionImage::name() const
{
  return FilterTraits<ITKCurvatureAnisotropicDiffusionImage>::name.str();
}

Uuid ITKCurvatureAnisotropicDiffusionImage::uuid() const
{
  return FilterTraits<ITKCurvatureAnisotropicDiffusionImage>::uuid;
}

std::string ITKCurvatureAnisotropicDiffusionImage::humanName() const
{
  return "ITK::Curvature Anisotropic Diffusion Image Filter";
}

Parameters ITKCurvatureAnisotropicDiffusionImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceParameter_Key, "ConductanceParameter", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceScalingUpdateInterval_Key, "ConductanceScalingUpdateInterval", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKCurvatureAnisotropicDiffusionImage::clone() const
{
  return std::make_unique<ITKCurvatureAnisotropicDiffusionImage>();
}

Result<OutputActions> ITKCurvatureAnisotropicDiffusionImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pTimeStepValue = filterArgs.value<float64>(k_TimeStep_Key);
  auto pConductanceParameterValue = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto pConductanceScalingUpdateIntervalValue = filterArgs.value<float64>(k_ConductanceScalingUpdateInterval_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKCurvatureAnisotropicDiffusionImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKCurvatureAnisotropicDiffusionImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pTimeStepValue = filterArgs.value<float64>(k_TimeStep_Key);
  auto pConductanceParameterValue = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto pConductanceScalingUpdateIntervalValue = filterArgs.value<float64>(k_ConductanceScalingUpdateInterval_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
