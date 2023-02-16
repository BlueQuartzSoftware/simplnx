#include "FindBoundingBoxFeaturesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindBoundingBoxFeatures.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundingBoxFeaturesFilter::name() const
{
  return FilterTraits<FindBoundingBoxFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundingBoxFeaturesFilter::className() const
{
  return FilterTraits<FindBoundingBoxFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundingBoxFeaturesFilter::uuid() const
{
  return FilterTraits<FindBoundingBoxFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundingBoxFeaturesFilter::humanName() const
{
  return "Find Biased Features (Bounding Box)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundingBoxFeaturesFilter::defaultTags() const
{
  return {"Generic", "Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindBoundingBoxFeaturesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalcByPhase_Key, "Apply Phase by Phase", "Whether to apply the biased Features algorithm per Ensemble", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The selected geometry in which to the (biased) features belong", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceFeaturesArrayPath_Key, "Surface Features", "Flag of 1 if Feature touches an outer surface or of 0 if it does not", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Feature belongs. Only required if Apply Phase by Phase is checked",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BiasedFeaturesArrayName_Key, "Biased Features", "Flag of 1 if Feature is biased or of 0 if it is not", "BiasedFeatures"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CalcByPhase_Key, k_PhasesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundingBoxFeaturesFilter::clone() const
{
  return std::make_unique<FindBoundingBoxFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundingBoxFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pCalcByPhaseValue = filterArgs.value<bool>(k_CalcByPhase_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pPhasesArrayPathValue = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  auto pBiasedFeaturesArrayNameValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> cellFeatureArrayDataPaths = {pCentroidsArrayPathValue, pSurfaceFeaturesArrayPathValue};
  if(pCalcByPhaseValue)
  {
    cellFeatureArrayDataPaths.push_back(pPhasesArrayPathValue);
  }
  if(dataStructure.validateNumberOfTuples(cellFeatureArrayDataPaths))
  {
    return {MakeErrorResult<OutputActions>(-7460, fmt::format("The selected input cell feature arrays have mismatching number of tuples. Make sure the selected centroids, surface features (and "
                                                              "phases) are created from the same geometry's cell data attribute matrix."))};
  }

  const usize numCells = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath).getNumberOfCells();
  const auto& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(pSurfaceFeaturesArrayPathValue);
  const usize numSurfaceFeatures = surfaceFeatures.getSize();
  if(numSurfaceFeatures != numCells)
  {
    return {MakeErrorResult<OutputActions>(-7461, fmt::format("The selected Image geometry '{}' and one or more of the input cell feature arrays have mismatching number of elements. Make sure the "
                                                              "selected input arrays were created for the selected Image geometry.",
                                                              pImageGeometryPath.toString()))};
  }

  auto action = std::make_unique<CreateArrayAction>(DataType::int8, surfaceFeatures.getTupleShape(), std::vector<usize>{1}, pBiasedFeaturesArrayNameValue);
  resultOutputActions.value().actions.push_back(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindBoundingBoxFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  FindBoundingBoxFeaturesInputValues inputValues;

  inputValues.CalcByPhase = filterArgs.value<bool>(k_CalcByPhase_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  inputValues.BiasedFeaturesArrayName = filterArgs.value<DataPath>(k_BiasedFeaturesArrayName_Key);

  return FindBoundingBoxFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
