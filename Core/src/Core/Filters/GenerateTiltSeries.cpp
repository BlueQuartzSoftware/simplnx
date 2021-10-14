#include "GenerateTiltSeries.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string GenerateTiltSeries::name() const
{
  return FilterTraits<GenerateTiltSeries>::name.str();
}

std::string GenerateTiltSeries::className() const
{
  return FilterTraits<GenerateTiltSeries>::className;
}

Uuid GenerateTiltSeries::uuid() const
{
  return FilterTraits<GenerateTiltSeries>::uuid;
}

std::string GenerateTiltSeries::humanName() const
{
  return "Generate Tilt Series";
}

Parameters GenerateTiltSeries::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_RotationAxis_Key, "Rotation Axis", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationLimits_Key, "Rotation Limits (Start, Stop, Increment) Degrees", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Resample Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "Input Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_OutputPrefix_Key, "DataContainer Prefix", "", "SomeString"));

  return params;
}

IFilter::UniquePointer GenerateTiltSeries::clone() const
{
  return std::make_unique<GenerateTiltSeries>();
}

Result<OutputActions> GenerateTiltSeries::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRotationAxisValue = filterArgs.value<ChoicesParameter::ValueType>(k_RotationAxis_Key);
  auto pRotationLimitsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationLimits_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputPrefix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateTiltSeriesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> GenerateTiltSeries::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRotationAxisValue = filterArgs.value<ChoicesParameter::ValueType>(k_RotationAxis_Key);
  auto pRotationLimitsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationLimits_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputPrefix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
