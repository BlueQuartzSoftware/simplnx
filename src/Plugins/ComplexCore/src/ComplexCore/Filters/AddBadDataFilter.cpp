#include "AddBadDataFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AddBadData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AddBadDataFilter::name() const
{
  return FilterTraits<AddBadDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AddBadDataFilter::className() const
{
  return FilterTraits<AddBadDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AddBadDataFilter::uuid() const
{
  return FilterTraits<AddBadDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AddBadDataFilter::humanName() const
{
  return "Add Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> AddBadDataFilter::defaultTags() const
{
  return {"Synthetic Building", "Misc"};
}

//------------------------------------------------------------------------------
Parameters AddBadDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_PoissonNoise_Key, "Add Random Noise", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_PoissonVolFraction_Key, "Volume Fraction of Random Noise", "", 0.0f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BoundaryNoise_Key, "Add Boundary Noise", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_BoundaryVolFraction_Key, "Volume Fraction of Boundary Noise", "", 0.0f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBEuclideanDistancesArrayPath_Key, "Boundary Euclidean Distances", "", DataPath{}, complex::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PoissonNoise_Key, k_PoissonVolFraction_Key, true);
  params.linkParameters(k_BoundaryNoise_Key, k_BoundaryVolFraction_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AddBadDataFilter::clone() const
{
  return std::make_unique<AddBadDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AddBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pPoissonNoiseValue = filterArgs.value<bool>(k_PoissonNoise_Key);
  auto pPoissonVolFractionValue = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  auto pBoundaryNoiseValue = filterArgs.value<bool>(k_BoundaryNoise_Key);
  auto pBoundaryVolFractionValue = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AddBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{

  AddBadDataInputValues inputValues;

  inputValues.PoissonNoise = filterArgs.value<bool>(k_PoissonNoise_Key);
  inputValues.PoissonVolFraction = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  inputValues.BoundaryNoise = filterArgs.value<bool>(k_BoundaryNoise_Key);
  inputValues.BoundaryVolFraction = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  inputValues.GBEuclideanDistancesArrayPath = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);

  return AddBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
