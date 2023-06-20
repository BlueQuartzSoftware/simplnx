#include "FindBoundaryElementFractionsFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::name() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::className() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryElementFractionsFilter::uuid() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::humanName() const
{
  return "Find Feature Boundary Element Fractions";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryElementFractionsFilter::defaultTags() const
{
  return {"Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryElementFractionsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Surface Elements",
                                                          "DataArray containing the number of neighboring Elements of a given Element that belong to a different Feature than itself", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureDataAMPath_Key, "Feature Data", "Parent Attribute Matrix for the Surface Element Fractions Array to be created in", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Data Object"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BoundaryCellFractionsArrayName_Key, "Surface Element Fractions",
                                                          "Name of created Data Array containing fraction of Elements belonging to the Feature that are \"surface\" Elements",
                                                          "Surface Element Fractions"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryElementFractionsFilter::clone() const
{
  return std::make_unique<FindBoundaryElementFractionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryElementFractionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pFeatureDataAMPathValue = filterArgs.value<DataPath>(k_FeatureDataAMPath_Key);
  auto pBoundaryCellFractionsArrayPathValue = filterArgs.value<std::string>(k_BoundaryCellFractionsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> numTuples = dataStructure.getDataAs<AttributeMatrix>(pFeatureDataAMPathValue)->getShape();
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, numTuples, std::vector<usize>{1}, pFeatureDataAMPathValue.createChildPath(pBoundaryCellFractionsArrayPathValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryElementFractionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key));
  auto& boundaryCells = dataStructure.getDataRefAs<Int8Array>(filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key));
  auto& boundaryCellFractions =
      dataStructure.getDataRefAs<Float32Array>(filterArgs.value<DataPath>(k_FeatureDataAMPath_Key).createChildPath(filterArgs.value<std::string>(k_BoundaryCellFractionsArrayName_Key)));

  usize totalPoints = featureIds.getNumberOfTuples();
  usize numFeatures = boundaryCellFractions.getNumberOfTuples();

  std::vector<float32> surfVoxCounts(numFeatures, 0);
  std::vector<float32> voxCounts(numFeatures, 0);

  for(usize j = 0; j < totalPoints; j++)
  {
    int32 gnum = featureIds[j];
    voxCounts[gnum]++;
    if(boundaryCells[j] > 0)
    {
      surfVoxCounts[gnum]++;
    }
  }
  for(usize i = 1; i < numFeatures; i++)
  {
    boundaryCellFractions[i] = surfVoxCounts[i] / voxCounts[i];
  }
  return {};
}
} // namespace complex
