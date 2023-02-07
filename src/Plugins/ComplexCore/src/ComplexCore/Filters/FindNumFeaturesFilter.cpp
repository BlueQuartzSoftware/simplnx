#include "FindNumFeaturesFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNumFeaturesFilter::name() const
{
  return FilterTraits<FindNumFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNumFeaturesFilter::className() const
{
  return FilterTraits<FindNumFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindNumFeaturesFilter::uuid() const
{
  return FilterTraits<FindNumFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNumFeaturesFilter::humanName() const
{
  return "Find Number of Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNumFeaturesFilter::defaultTags() const
{
  return {"Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindNumFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Objects: Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Array specifying which Ensemble each Feature belongs",
                                                          DataPath({"DataContainer", "FeatureData", "Phases"}), complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Objects: Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "The number of Features that belong to each Ensemble", DataPath({"Number of Features"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNumFeaturesFilter::clone() const
{
  return std::make_unique<FindNumFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNumFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* featureData = dataStructure.getDataAs<AttributeMatrix>(pNumFeaturesArrayPathValue.getParent());
  if(featureData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-47630, fmt::format("Could not find selected feature Attribute Matrix at path '{}'", pNumFeaturesArrayPathValue.getParent().toString()))};
  }

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, featureData->getShape(), std::vector<usize>{1}, pNumFeaturesArrayPathValue);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindNumFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  const auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key));
  auto& numFeaturesArrayRef = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key));

  for(usize index = 1; index < featurePhasesArrayRef.getNumberOfTuples(); index++)
  {
    numFeaturesArrayRef[featurePhasesArrayRef[index]]++;
  }
  return {};
}
} // namespace complex
