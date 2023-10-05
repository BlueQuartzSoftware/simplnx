#include "FindFeatureClusteringFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindFeatureClustering.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <random>

using namespace complex;

namespace complex
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins for RDF", "Number of bins to split the RDF", 1));
  params.insert(std::make_unique<Int32Parameter>(k_PhaseNumber_Key, "Phase Index", "Ensemble number for which to calculate the RDF and clustering list", 1));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveBiasedFeatures_Key, "Remove Biased Features", "Remove the biased features", false));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SetRandomSeed_Key, "Set Random Seed", "When checked, allows the user to set the seed value used to randomly generate the points in the RDF", true));
  params.insert(std::make_unique<UInt64Parameter>(k_SeedValue_Key, "Seed Value", "The seed value used to randomly generate the points in the RDF", std::mt19937::default_seed));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "Diameter of a sphere with the same volume as the Feature", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BiasedFeaturesArrayPath_Key, "Biased Features",
                                                          "Specifies which features are biased and therefor should be removed if the Remove Biased Features option is on", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix",
                                                                    "The path to the cell ensemble attribute matrix where the RDF and RDF min and max distance arrays will be stored", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_ClusteringListArrayName_Key, "Clustering List", "Distance of each Features's centroid to ever other Features's centroid", "ClusteringList"));
  params.insertSeparator(Parameters::Separator{"Created Cell Ensemble Data"});
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
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pClusteringListArrayNameValue = filterArgs.value<std::string>(k_ClusteringListArrayName_Key);
  auto pRDFArrayNameValue = filterArgs.value<std::string>(k_RDFArrayName_Key);
  auto pMaxMinArrayNameValue = filterArgs.value<std::string>(k_MaxMinArrayName_Key);

  const DataPath clusteringListPath = pFeaturePhasesArrayPathValue.getParent().createChildPath(pClusteringListArrayNameValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
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
  const std::vector<usize> tupleShape = cellEnsembleAM.getShape();
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

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureClusteringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  FindFeatureClusteringInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.PhaseNumber = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.RemoveBiasedFeatures = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  inputValues.UseSeed = filterArgs.value<bool>(k_SetRandomSeed_Key);
  inputValues.SeedValue = filterArgs.value<uint64>(k_SeedValue_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.BiasedFeaturesArrayPath = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.ClusteringListArrayName = inputValues.FeaturePhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_ClusteringListArrayName_Key));
  inputValues.RDFArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_RDFArrayName_Key));
  inputValues.MaxMinArrayName = inputValues.CellEnsembleAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_MaxMinArrayName_Key));

  return FindFeatureClustering(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
