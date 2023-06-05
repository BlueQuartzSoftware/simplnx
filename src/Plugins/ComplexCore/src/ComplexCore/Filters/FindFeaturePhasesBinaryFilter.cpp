#include "FindFeaturePhasesBinaryFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinaryFilter::name() const
{
  return FilterTraits<FindFeaturePhasesBinaryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinaryFilter::className() const
{
  return FilterTraits<FindFeaturePhasesBinaryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeaturePhasesBinaryFilter::uuid() const
{
  return FilterTraits<FindFeaturePhasesBinaryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeaturePhasesBinaryFilter::humanName() const
{
  return "Find Feature Phases Binary";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeaturePhasesBinaryFilter::defaultTags() const
{
  return {"Generic", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeaturePhasesBinaryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Ensemble Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeaturePhasesBinaryFilter::clone() const
{
  return std::make_unique<FindFeaturePhasesBinaryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeaturePhasesBinaryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
     auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pFeaturePhasesArrayPathValue);
     resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeaturePhasesBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto featureIdsArray = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key));
  auto featurePhasesArray = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key));

  std::unique_ptr<MaskCompare> goodVoxelsMask = nullptr;
  try
  {
    goodVoxelsMask = InstantiateMaskCompare(dataStructure, filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key));
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key).toString());
    return MakeErrorResult(-53800, message);
  }

  usize totalPoints = featureIdsArray.getNumberOfTuples();

  for(usize i = 0; i < totalPoints; i++)
  {
    if(goodVoxelsMask->isTrue(i))
    {
      featurePhasesArray[featureIdsArray[i]] = 1;
    }
    else
    {
      featurePhasesArray[featureIdsArray[i]] = 0;
    }
  }

  return {};
}
} // namespace complex
