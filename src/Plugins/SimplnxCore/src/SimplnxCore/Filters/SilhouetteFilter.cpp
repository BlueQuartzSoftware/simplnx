#include "SilhouetteFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/Silhouette.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/KUtilities.hpp"

using namespace nx::core;

namespace
{
const std::string k_MaskName = "temp_mask";
}

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

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Optional Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean}));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(KUtilities::DistanceMetric::Euclidean),
                                         ChoicesParameter::Choices{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Required Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Silhouette", "The DataPath to the input DataArray", DataPath{}, nx::core::GetAllNumericTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cluster Ids", "The DataPath to the DataArray that specifies which cluster each point belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SilhouetteArrayPath_Key, "Silhouette", "The DataPath to the calculated output Silhouette array values", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SilhouetteFilter::clone() const
{
  return std::make_unique<SilhouetteFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SilhouetteFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSilhouetteArrayPathValue = filterArgs.value<DataPath>(k_SilhouetteArrayPath_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto clusterArray = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);
  auto clusterIds = dataStructure.getDataAs<IDataArray>(pFeatureIdsArrayPathValue);
  if(clusterArray->getNumberOfTuples() != clusterIds->getNumberOfTuples())
  {
    return MakePreflightErrorResult(-8976, fmt::format("The the number of tuples for {} ({}) do not match the number of tuples for {} ({})", clusterArray->getName(), clusterArray->getNumberOfTuples(),
                                                       clusterIds->getName(), clusterIds->getNumberOfTuples()));
  }

  if(!pUseMaskValue)
  {
    DataPath tempPath = DataPath({k_MaskName});
    {
      auto createAction = std::make_unique<CreateArrayAction>(DataType::boolean, clusterArray->getTupleShape(), std::vector<usize>{1}, tempPath);
      resultOutputActions.value().appendAction(std::move(createAction));
    }

    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::float64, clusterArray->getTupleShape(), std::vector<usize>{1}, pSilhouetteArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SilhouetteFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  if(!filterArgs.value<bool>(k_UseMask_Key))
  {
    maskPath = DataPath({k_MaskName});
    dataStructure.getDataRefAs<BoolArray>(maskPath).fill(true);
  }

  SilhouetteInputValues inputValues;

  inputValues.DistanceMetric = static_cast<KUtilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.ClusteringArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = maskPath;
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.SilhouetteArrayPath = filterArgs.value<DataPath>(k_SilhouetteArrayPath_Key);

  return Silhouette(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DistanceMetricKey = "DistanceMetric";
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_SilhouetteArrayPathKey = "SilhouetteArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> SilhouetteFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SilhouetteFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DistanceMetricKey, k_DistanceMetric_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_SilhouetteArrayPathKey, k_SilhouetteArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
