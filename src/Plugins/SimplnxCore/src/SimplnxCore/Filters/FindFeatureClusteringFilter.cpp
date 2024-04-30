#include "FindFeatureClusteringFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FindFeatureClustering.hpp"

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

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindFeatureClusteringFilter::name() const
{
  return FilterTraits<FindFeatureClusteringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureClusteringFilter::className() const
{
  return FilterTraits<FindFeatureClusteringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureClusteringFilter::uuid() const
{
  return FilterTraits<FindFeatureClusteringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureClusteringFilter::humanName() const
{
  return "Find Feature Clustering";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureClusteringFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureClusteringFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins for RDF", "Number of bins to split the RDF", 1));
  params.insert(std::make_unique<Int32Parameter>(k_PhaseNumber_Key, "Phase Index", "Ensemble number for which to calculate the RDF and clustering list", 1));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveBiasedFeatures_Key, "Remove Biased Features", "Remove the biased features", false));

  params.insertSeparator(Parameters::Separator{"Seeded Randomness"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SetRandomSeed_Key, "Set Random Seed", "When checked, allows the user to set the seed value used to randomly generate the points in the RDF", true));
  params.insert(std::make_unique<UInt64Parameter>(k_SeedValue_Key, "Seed Value", "The seed value used to randomly generate the points in the RDF", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "FindFeatureClustering SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "Diameter of a sphere with the same volume as the Feature", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BiasedFeaturesArrayPath_Key, "Biased Features",
                                                          "Specifies which features are biased and therefor should be removed if the Remove Biased Features option is on", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellEnsembleAttributeMatrixPath_Key, "Cell Ensemble Attribute Matrix",
                                                                    "The path to the cell ensemble attribute matrix where the RDF and RDF min and max distance arrays will be stored", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ClusteringListArrayName_Key, "Clustering List", "Distance of each Feature's centroid to every other Feature's centroid", "ClusteringList"));
  params.insertSeparator(Parameters::Separator{"Created Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_RDFArrayName_Key, "Radial Distribution Function", "A histogram of the normalized frequency at each bin", "RDF"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaxMinArrayName_Key, "Max and Min Separation Distances", "The max and min distance found between Features", "RDFMaxMinDistances"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RemoveBiasedFeatures_Key, k_BiasedFeaturesArrayPath_Key, true);
  params.linkParameters(k_SetRandomSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureClusteringFilter::clone() const
{
  return std::make_unique<FindFeatureClusteringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureClusteringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
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

  std::vector<DataPath> featureDataArrays = {pEquivalentDiametersArrayPathValue, pFeaturePhasesArrayPathValue, pCentroidsArrayPathValue};
  if(pRemoveBiasedFeaturesValue)
  {
    featureDataArrays.push_back(filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(featureDataArrays);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-9750, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& cellEnsembleAM = dataStructure.getDataRefAs<AttributeMatrix>(pCellEnsembleAttributeMatrixNameValue);
  const std::vector<usize>& tupleShape = cellEnsembleAM.getShape();
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
    auto createArrayAction = std::make_unique<CreateNeighborListAction>(DataType::float32, cellFeatureAM.getNumTuples(), featureAttributeMatrixPath.createChildPath(pClusteringListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureClusteringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto seed = filterArgs.value<uint64>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_SetRandomSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  FindFeatureClusteringInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.PhaseNumber = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.RemoveBiasedFeatures = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  inputValues.SeedValue = seed;
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.BiasedFeaturesArrayPath = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  inputValues.ClusteringListArrayName = inputValues.FeaturePhasesArrayPath.replaceName(filterArgs.value<std::string>(k_ClusteringListArrayName_Key));
  inputValues.RDFArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_RDFArrayName_Key));
  inputValues.MaxMinArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_MaxMinArrayName_Key));

  return FindFeatureClustering(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_NumberOfBinsKey = "NumberOfBins";
constexpr StringLiteral k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteral k_RemoveBiasedFeaturesKey = "RemoveBiasedFeatures";
constexpr StringLiteral k_UseRandomSeedKey = "UseRandomSeed";
constexpr StringLiteral k_RandomSeedValueKey = "RandomSeedValue";
constexpr StringLiteral k_EquivalentDiametersArrayPathKey = "EquivalentDiametersArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteral k_BiasedFeaturesArrayPathKey = "BiasedFeaturesArrayPath";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
constexpr StringLiteral k_ClusteringListArrayNameKey = "ClusteringListArrayName";
constexpr StringLiteral k_NewEnsembleArrayArrayNameKey = "NewEnsembleArrayArrayName";
constexpr StringLiteral k_MaxMinArrayNameKey = "MaxMinArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> FindFeatureClusteringFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindFeatureClusteringFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumberOfBinsKey, k_NumberOfBins_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_RemoveBiasedFeaturesKey, k_RemoveBiasedFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseRandomSeedKey, k_SetRandomSeed_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64FilterParameterConverter>(args, json, SIMPL::k_RandomSeedValueKey, k_SeedValue_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayPathKey, k_EquivalentDiametersArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_BiasedFeaturesArrayPathKey, k_BiasedFeaturesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey,
                                                                                                                         k_CellEnsembleAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ClusteringListArrayNameKey, k_ClusteringListArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NewEnsembleArrayArrayNameKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MaxMinArrayNameKey, k_MaxMinArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
