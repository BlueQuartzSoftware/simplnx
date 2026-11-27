#include "ComputeFeatureClusteringFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureClustering.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidNumberOfBins = -54099;

Result<> ValidateNumberOfBins(int32 numberOfBins)
{
  if(numberOfBins < 1)
  {
    return MakeErrorResult(k_InvalidNumberOfBins, fmt::format("Number of Bins for RDF must be at least 1, but was {}.", numberOfBins));
  }
  return {};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureClusteringFilter::name() const
{
  return FilterTraits<ComputeFeatureClusteringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureClusteringFilter::className() const
{
  return FilterTraits<ComputeFeatureClusteringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureClusteringFilter::uuid() const
{
  return FilterTraits<ComputeFeatureClusteringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureClusteringFilter::humanName() const
{
  return "Compute Feature Clustering";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureClusteringFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Compute"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureClusteringFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<Int32ParameterType>(k_NumberOfBins_Key, "Number of Bins for RDF", "Number of bins to split the RDF", 1));
  params.insert(std::make_unique<Int32ParameterType>(k_PhaseNumber_Key, "Phase Index", "Ensemble number for which to calculate the RDF and clustering list", 1));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveBiasedFeatures_Key, "Remove Biased Features", "Remove the biased features", false));

  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SetRandomSeed_Key, "Set Random Seed", "When checked, allows the user to set the seed value used to randomly generate the points in the RDF", true));
  params.insert(std::make_unique<UInt64ParameterType>(k_SeedValue_Key, "Seed Value", "The seed value used to randomly generate the points in the RDF", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "ComputeFeatureClustering SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BiasedFeaturesArrayPath_Key, "Biased Features",
                                                          "Specifies which features are biased and therefore should be removed if the Remove Biased Features option is on; True values removed",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellEnsembleAttributeMatrixPath_Key, "Cell Ensemble Attribute Matrix",
                                                                    "The path to the cell ensemble attribute matrix where the RDF and RDF min and max distance arrays will be stored", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ClusteringListArrayName_Key, "Clustering List", "Distance of each Feature's centroid to every other Feature's centroid", "ClusteringList"));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_RDFArrayName_Key, "Radial Distribution Function", "A histogram of the normalized frequency at each bin", "RDF"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaxMinArrayName_Key, "Max and Min Separation Distances", "The max and min distance found between Features", "RDFMaxMinDistances"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RemoveBiasedFeatures_Key, k_BiasedFeaturesArrayPath_Key, true);
  params.linkParameters(k_SetRandomSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureClusteringFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeFeatureClusteringFilter::clone() const
{
  return std::make_unique<ComputeFeatureClusteringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureClusteringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                       const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  Result<> binCountResult = ValidateNumberOfBins(pNumberOfBinsValue);
  if(binCountResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(binCountResult), {})};
  }
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  auto pClusteringListArrayNameValue = filterArgs.value<std::string>(k_ClusteringListArrayName_Key);
  auto pRDFArrayNameValue = filterArgs.value<std::string>(k_RDFArrayName_Key);
  auto pMaxMinArrayNameValue = filterArgs.value<std::string>(k_MaxMinArrayName_Key);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  const DataPath clusteringListPath = pFeaturePhasesArrayPathValue.replaceName(pClusteringListArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> featureDataArrays = {pFeaturePhasesArrayPathValue, pCentroidsArrayPathValue};
  if(pRemoveBiasedFeaturesValue)
  {
    featureDataArrays.push_back(filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(featureDataArrays);
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-9750, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& cellEnsembleAM = dataStructure.getDataRefAs<AttributeMatrix>(pCellEnsembleAttributeMatrixNameValue);
  const ShapeType& tupleShape = cellEnsembleAM.getShape();
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{static_cast<usize>(pNumberOfBinsValue)},
                                                                 pCellEnsembleAttributeMatrixNameValue.createChildPath(pRDFArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{2}, pCellEnsembleAttributeMatrixNameValue.createChildPath(pMaxMinArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    const DataPath featureAttributeMatrixPath = pFeaturePhasesArrayPathValue.getParent();
    const auto& cellFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(featureAttributeMatrixPath);
    auto createArrayAction = std::make_unique<CreateNeighborListAction>(DataType::float32, cellFeatureAM.getShape(), featureAttributeMatrixPath.createChildPath(pClusteringListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureClusteringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto seed = filterArgs.value<uint64>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_SetRandomSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  ComputeFeatureClusteringInputValues inputValues;

  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.number_of_bins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.phase_number = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.remove_biased_features = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  inputValues.seed_value = seed;
  inputValues.feature_phases_array_path = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.centroids_array_path = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.biased_features_array_path = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.cell_ensemble_attribute_matrix_name = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  inputValues.clustering_list_array_name = inputValues.feature_phases_array_path.replaceName(filterArgs.value<std::string>(k_ClusteringListArrayName_Key));
  inputValues.rdf_array_name = inputValues.cell_ensemble_attribute_matrix_name.createChildPath(filterArgs.value<std::string>(k_RDFArrayName_Key));
  inputValues.max_min_array_name = inputValues.cell_ensemble_attribute_matrix_name.createChildPath(filterArgs.value<std::string>(k_MaxMinArrayName_Key));

  return ComputeFeatureClustering(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_NumberOfBinsKey = "NumberOfBins";
constexpr StringLiteralType k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteralType k_RemoveBiasedFeaturesKey = "RemoveBiasedFeatures";
constexpr StringLiteralType k_UseRandomSeedKey = "UseRandomSeed";
constexpr StringLiteralType k_RandomSeedValueKey = "RandomSeedValue";
constexpr StringLiteralType k_EquivalentDiametersArrayPathKey = "EquivalentDiametersArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteralType k_BiasedFeaturesArrayPathKey = "BiasedFeaturesArrayPath";
constexpr StringLiteralType k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
constexpr StringLiteralType k_ClusteringListArrayNameKey = "ClusteringListArrayName";
constexpr StringLiteralType k_NewEnsembleArrayArrayNameKey = "NewEnsembleArrayArrayName";
constexpr StringLiteralType k_MaxMinArrayNameKey = "MaxMinArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeFeatureClusteringFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureClusteringFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumberOfBinsKey, k_NumberOfBins_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_RemoveBiasedFeaturesKey, k_RemoveBiasedFeatures_Key));
  Result<> useSeedResult = simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseRandomSeedKey, k_SetRandomSeed_Key);
  if(useSeedResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(useSeedResult));
  }
  Result<> seedResult = simpl_conversion::ConvertParameter<simpl_conversion::StringToIntFilterParameterConverter<uint64>>(args, json, simpl::k_RandomSeedValueKey, k_SeedValue_Key);
  if(seedResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(seedResult));
  }
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_EquivalentDiametersArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_BiasedFeaturesArrayPathKey, k_BiasedFeaturesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellEnsembleAttributeMatrixNameKey,
                                                                                                                         k_CellEnsembleAttributeMatrixPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_ClusteringListArrayNameKey, k_ClusteringListArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_NewEnsembleArrayArrayNameKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_MaxMinArrayNameKey, k_MaxMinArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
