#include "AddBadData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AddBadData::name() const
{
  return FilterTraits<AddBadData>::name.str();
}

//------------------------------------------------------------------------------
std::string AddBadData::className() const
{
  return FilterTraits<AddBadData>::className;
}

//------------------------------------------------------------------------------
Uuid AddBadData::uuid() const
{
  return FilterTraits<AddBadData>::uuid;
}

//------------------------------------------------------------------------------
std::string AddBadData::humanName() const
{
  return "Add Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> AddBadData::defaultTags() const
{
  return {"#Synthetic Building", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters AddBadData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PoissonNoise_Key, "Add Random Noise", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_PoissonVolFraction_Key, "Volume Fraction of Random Noise", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BoundaryNoise_Key, "Add Boundary Noise", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_BoundaryVolFraction_Key, "Volume Fraction of Boundary Noise", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBEuclideanDistancesArrayPath_Key, "Boundary Euclidean Distances", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PoissonNoise_Key, k_PoissonVolFraction_Key, true);
  params.linkParameters(k_BoundaryNoise_Key, k_BoundaryVolFraction_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AddBadData::clone() const
{
  return std::make_unique<AddBadData>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AddBadData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPoissonNoiseValue = filterArgs.value<bool>(k_PoissonNoise_Key);
  auto pPoissonVolFractionValue = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  auto pBoundaryNoiseValue = filterArgs.value<bool>(k_BoundaryNoise_Key);
  auto pBoundaryVolFractionValue = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AddBadDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AddBadData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPoissonNoiseValue = filterArgs.value<bool>(k_PoissonNoise_Key);
  auto pPoissonVolFractionValue = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  auto pBoundaryNoiseValue = filterArgs.value<bool>(k_BoundaryNoise_Key);
  auto pBoundaryVolFractionValue = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
