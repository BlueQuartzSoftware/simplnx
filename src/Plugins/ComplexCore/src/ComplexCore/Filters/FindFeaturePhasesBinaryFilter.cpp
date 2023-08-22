#include "FindFeaturePhasesBinaryFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
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
  return {className(), "Generic", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeaturePhasesBinaryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Data Array that specifies if the Cell is to be counted in the algorithm", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellDataAMPath_Key, "Cell Data Attribute Matrix",
                                                                    "The Cell Data Attribute Matrix within the Image Geometry where the Binary Phases Array will be created", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayName_Key, "Binary Feature Phases Array Name", "Created Data Array name to specify to which Ensemble each Feature belongs",
                                                          "Binary Phases Array"));

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
  auto pCellDataAMPathValue = filterArgs.value<DataPath>(k_CellDataAMPath_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto cellDataAM = dataStructure.getDataAs<AttributeMatrix>(pCellDataAMPathValue);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, cellDataAM->getShape(), std::vector<usize>{1}, pCellDataAMPathValue.createChildPath(pFeaturePhasesArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeaturePhasesBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto featureIdsArray = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key));
  auto featurePhasesArray = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_CellDataAMPath_Key).createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key)));

  std::unique_ptr<MaskCompare> goodVoxelsMask;
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
