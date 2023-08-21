#include "FindNeighborhoodsFilter.hpp"

#include "Algorithms/FindNeighborhoods.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::name() const
{
  return FilterTraits<FindNeighborhoodsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::className() const
{
  return FilterTraits<FindNeighborhoodsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindNeighborhoodsFilter::uuid() const
{
  return FilterTraits<FindNeighborhoodsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::humanName() const
{
  return "Find Feature Neighborhoods";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighborhoodsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindNeighborhoodsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_MultiplesOfAverage_Key, "Multiples of Average Diameter", "Defines the search radius to use when looking for 'neighboring' Features", 1.0F));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "Path to the array specifying the diameter of a sphere with the same volume as the Feature",
      DataPath({"CellFeatureData", "EquivalentDiameters"}), ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Path to the array specifying to which Ensemble each Feature belongs",
                                                          DataPath({"CellFeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "Path to the array specifying the X, Y, Z coordinates of Feature center of mass",
                                                          DataPath({"CellFeatureData", "Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborhoodsArrayName_Key, "Neighborhoods",
                                                          "Number of Features that have their centroid within the user specified multiple of equivalent sphere diameters from each Feature",
                                                          "Neighborhoods"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborhoodListArrayName_Key, "NeighborhoodList",
                                                          "List of the Features whose centroids are within the user specified multiple of equivalent sphere diameter from each Feature",
                                                          "NeighborhoodList"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighborhoodsFilter::clone() const
{
  return std::make_unique<FindNeighborhoodsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighborhoodsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pMultiplesOfAverageValue = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pNeighborhoodsArrayNameValue = filterArgs.value<std::string>(k_NeighborhoodsArrayName_Key);
  auto pNeighborhoodListArrayNameValue = filterArgs.value<std::string>(k_NeighborhoodListArrayName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  // Get the Feature Phases Array and get its TupleShape
  if(!dataStructure.validateNumberOfTuples({pEquivalentDiametersArrayPathValue, pFeaturePhasesArrayPathValue, pCentroidsArrayPathValue}))
  {
    return MakePreflightErrorResult(-5730, fmt::format("One or more of the selected input feature data arrays (equivalent diameters, phases, and centroids) has mismatching tuples. Make sure you "
                                                       "select the input arrays from the cell feature attribute matrix."));
  }
  const auto* cellFeatureData = dataStructure.getDataAs<AttributeMatrix>(pFeaturePhasesArrayPathValue.getParent());
  if(cellFeatureData == nullptr)
  {
    return MakePreflightErrorResult(
        -5731,
        fmt::format(
            "The selected input feature data arrays (equivalent diameters, phases, and centroids) are not located in an attribute matrix. Make sure you have selected the input arrays located in the "
            "cell feature attribute matrix of the selected geometry"));
  }

  // Create the Neighborhoods Array in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, cellFeatureData->getShape(), std::vector<usize>{1ULL},
                                                      pFeaturePhasesArrayPathValue.getParent().createChildPath(pNeighborhoodsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the NeighborList Output NeighborList in the Feature Attribute Matrix
  {
    auto action =
        std::make_unique<CreateNeighborListAction>(DataType::int32, cellFeatureData->getNumTuples(), pFeaturePhasesArrayPathValue.getParent().createChildPath(pNeighborhoodListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindNeighborhoodsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  FindNeighborhoodsInputValues inputValues;

  inputValues.MultiplesOfAverage = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.NeighborhoodsArrayName = inputValues.FeaturePhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_NeighborhoodsArrayName_Key));
  inputValues.NeighborhoodListArrayName = inputValues.FeaturePhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_NeighborhoodListArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindNeighborhoods(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
