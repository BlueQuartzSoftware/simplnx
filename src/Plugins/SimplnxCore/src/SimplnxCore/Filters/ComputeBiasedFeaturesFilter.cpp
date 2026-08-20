#include "ComputeBiasedFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeBiasedFeatures.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBiasedFeaturesFilter::name() const
{
  return FilterTraits<ComputeBiasedFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBiasedFeaturesFilter::className() const
{
  return FilterTraits<ComputeBiasedFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBiasedFeaturesFilter::uuid() const
{
  return FilterTraits<ComputeBiasedFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBiasedFeaturesFilter::humanName() const
{
  return "Compute Biased Features (Bounding Box)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBiasedFeaturesFilter::defaultTags() const
{
  return {className(), "Generic", "Spatial"};
}

//------------------------------------------------------------------------------
Parameters ComputeBiasedFeaturesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalcByPhase_Key, "Apply Phase by Phase", "Whether to apply the biased Features algorithm per Ensemble", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The selected geometry in which to the (biased) features belong", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceFeaturesArrayPath_Key, "Surface Features", "Flag of 1 if Feature touches an outer surface or of 0 if it does not", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Feature belongs. Only required if Apply Phase by Phase is checked",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BiasedFeaturesArrayName_Key, "Biased Features", "Flag of 1 if Feature is biased or of 0 if it is not", "BiasedFeatures"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CalcByPhase_Key, k_PhasesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBiasedFeaturesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBiasedFeaturesFilter::clone() const
{
  return std::make_unique<ComputeBiasedFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBiasedFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCalcByPhaseValue = filterArgs.value<bool>(k_CalcByPhase_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pPhasesArrayPathValue = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  auto pBiasedFeaturesArrayNameValue = filterArgs.value<std::string>(k_BiasedFeaturesArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> cellFeatureArrayDataPaths = {pCentroidsArrayPathValue, pSurfaceFeaturesArrayPathValue};
  if(pCalcByPhaseValue)
  {
    cellFeatureArrayDataPaths.push_back(pPhasesArrayPathValue);
  }
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(cellFeatureArrayDataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-7460, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // validateNumberOfTuples() only checks that the selected arrays AGREE on their tuple count, so a
  // Feature AttributeMatrix with zero tuples satisfies it. That input is malformed - a Feature array
  // always carries at least the index-0 "unassigned" Feature - and it is also unsafe: with Apply
  // Phase by Phase on, the algorithm derives its phase-loop bound from
  // *std::max_element(phases.begin(), phases.end()), and std::max_element returns end() on an empty
  // range, so the dereference reads one past the array. Reject it here instead.
  const auto& centroidsArray = dataStructure.getDataRefAs<IDataArray>(pCentroidsArrayPathValue);
  if(centroidsArray.getNumberOfTuples() == 0)
  {
    return {MakeErrorResult<OutputActions>(-7461, fmt::format("The selected Feature arrays must contain at least one tuple, but '{}' contains {} tuples. Feature arrays carry the unassigned Feature "
                                                              "at index 0, so a tuple count of zero means no Feature data has been computed for this geometry.",
                                                              pCentroidsArrayPathValue.toString(), centroidsArray.getNumberOfTuples()))};
  }

  const auto& surfaceFeaturesMaskArray = dataStructure.getDataRefAs<IDataArray>(pSurfaceFeaturesArrayPathValue);
  // The output is explicitly initialized to false. Both algorithm paths begin with their own
  // fill(false), but a degenerate geometry with a dimension of 0 satisfies neither the 3D nor the
  // 2D dispatch test, so neither fill runs. Without a fill value here the DataStore is allocated
  // with `new bool[n]` and left indeterminate, which would hand the user a garbage mask on that
  // path. DREAM3D 6.5.171 passed an explicit `false` init value for the same reason.
  auto action = std::make_unique<CreateArrayAction>(DataType::boolean, surfaceFeaturesMaskArray.getTupleShape(), std::vector<usize>{1},
                                                    pCentroidsArrayPathValue.replaceName(pBiasedFeaturesArrayNameValue), CreateArrayAction::k_DefaultDataFormat, "false");
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeBiasedFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeBiasedFeaturesInputValues inputValues;

  inputValues.CalcByPhase = filterArgs.value<bool>(k_CalcByPhase_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  inputValues.BiasedFeaturesArrayName = inputValues.CentroidsArrayPath.replaceName(filterArgs.value<std::string>(k_BiasedFeaturesArrayName_Key));

  return ComputeBiasedFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CalcByPhaseKey = "CalcByPhase";
constexpr StringLiteral k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteral k_SurfaceFeaturesArrayPathKey = "SurfaceFeaturesArrayPath";
constexpr StringLiteral k_PhasesArrayPathKey = "PhasesArrayPath";
constexpr StringLiteral k_BiasedFeaturesArrayNameKey = "BiasedFeaturesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeBiasedFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeBiasedFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_CalcByPhaseKey, k_CalcByPhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceFeaturesArrayPathKey, k_SurfaceFeaturesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_PhasesArrayPathKey, k_PhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_BiasedFeaturesArrayNameKey, k_BiasedFeaturesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
