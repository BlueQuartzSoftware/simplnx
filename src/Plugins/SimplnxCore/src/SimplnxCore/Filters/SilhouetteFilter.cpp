#include "SilhouetteFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/Silhouette.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string SilhouetteFilter::name() const
{
  return FilterTraits<SilhouetteFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SilhouetteFilter::className() const
{
  return FilterTraits<SilhouetteFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SilhouetteFilter::uuid() const
{
  return FilterTraits<SilhouetteFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SilhouetteFilter::humanName() const
{
  return "Silhouette";
}

//------------------------------------------------------------------------------
std::vector<std::string> SilhouetteFilter::defaultTags() const
{
  return {className(), "DREAM3D Review", "Clustering"};
}

//------------------------------------------------------------------------------
Parameters SilhouetteFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(
      k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(cluster_utilities::DistanceMetric::Euclidean),
      ChoicesParameter::ChoicesType{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // Choice order matches ClusterUtilities::DistanceMetric values.

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the boolean or uint8 mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Silhouette", "The DataPath to the input DataArray", DataPath{}, nx::core::GetAllNumericTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cluster Ids", "The DataPath to the DataArray that specifies which cluster each point belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SilhouetteArrayPath_Key, "Silhouette", "The DataPath to the calculated output Silhouette array values", DataPath{}));

  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SilhouetteFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType SilhouetteFilter::clone() const
{
  return std::make_unique<SilhouetteFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SilhouetteFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                         const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSilhouetteArrayPathValue = filterArgs.value<DataPath>(k_SilhouetteArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* clusterArrayPtr = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);
  const auto* clusterIdsPtr = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPathValue);
  if(clusterArrayPtr->getNumberOfTuples() != clusterIdsPtr->getNumberOfTuples())
  {
    return MakePreflightErrorResult(-8976, fmt::format("The the number of tuples for {} ({}) do not match the number of tuples for {} ({})", clusterArrayPtr->getName(),
                                                       clusterArrayPtr->getNumberOfTuples(), clusterIdsPtr->getName(), clusterIdsPtr->getNumberOfTuples()));
  }

  if(pUseMaskValue && !CheckArraysHaveSameTupleCount(dataStructure, {pSelectedArrayPathValue, pMaskArrayPathValue, pFeatureIdsArrayPathValue}))
  {
    const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(pMaskArrayPathValue);
    return MakePreflightErrorResult(-8977, fmt::format("Attribute Array to Silhouette DataArray '{}' ({} tuples), Mask Array DataArray '{}' ({} tuples), and Cluster Ids DataArray '{}' ({} tuples) "
                                                       "must have the same number of tuples.",
                                                       pSelectedArrayPathValue.toString(), clusterArrayPtr->getNumberOfTuples(), pMaskArrayPathValue.toString(), maskArray.getNumberOfTuples(),
                                                       pFeatureIdsArrayPathValue.toString(), clusterIdsPtr->getNumberOfTuples()));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::float64, clusterArrayPtr->getTupleShape(), std::vector<usize>{1}, pSilhouetteArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> SilhouetteFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  SilhouetteInputValues inputValues;

  inputValues.distance_metric = static_cast<cluster_utilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.clustering_array_path = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.silhouette_array_path = filterArgs.value<DataPath>(k_SilhouetteArrayPath_Key);

  return Silhouette(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_DistanceMetricKey = "DistanceMetric";
constexpr StringLiteralType k_UseMaskKey = "UseMask";
constexpr StringLiteralType k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteralType k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_SilhouetteArrayPathKey = "SilhouetteArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> SilhouetteFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SilhouetteFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DistanceMetricKey, k_DistanceMetric_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseMaskKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationFilterParameterConverter>(args, json, simpl::k_SilhouetteArrayPathKey, k_SilhouetteArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
