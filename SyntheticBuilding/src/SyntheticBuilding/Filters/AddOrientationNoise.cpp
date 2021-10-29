#include "AddOrientationNoise.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AddOrientationNoise::name() const
{
  return FilterTraits<AddOrientationNoise>::name.str();
}

//------------------------------------------------------------------------------
std::string AddOrientationNoise::className() const
{
  return FilterTraits<AddOrientationNoise>::className;
}

//------------------------------------------------------------------------------
Uuid AddOrientationNoise::uuid() const
{
  return FilterTraits<AddOrientationNoise>::uuid;
}

//------------------------------------------------------------------------------
std::string AddOrientationNoise::humanName() const
{
  return "Add Orientation Noise";
}

//------------------------------------------------------------------------------
std::vector<std::string> AddOrientationNoise::defaultTags() const
{
  return {"#Synthetic Building", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters AddOrientationNoise::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_Magnitude_Key, "Magnitude of Orientation Noise (Degrees)", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AddOrientationNoise::clone() const
{
  return std::make_unique<AddOrientationNoise>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AddOrientationNoise::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMagnitudeValue = filterArgs.value<float32>(k_Magnitude_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AddOrientationNoiseAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AddOrientationNoise::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMagnitudeValue = filterArgs.value<float32>(k_Magnitude_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
