#include "ComputeKMeansFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeKMeans.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/KUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace
{
const std::string k_MaskName = "temp_mask";
}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeKMeansFilter::name() const
{
  return FilterTraits<ComputeKMeansFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeKMeansFilter::className() const
{
  return FilterTraits<ComputeKMeansFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeKMeansFilter::uuid() const
{
  return FilterTraits<ComputeKMeansFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeKMeansFilter::humanName() const
{
  return "Compute K Means";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeKMeansFilter::defaultTags() const
{
  return {className(), "DREAM3D Review", "Clustering"};
}

//------------------------------------------------------------------------------
Parameters ComputeKMeansFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "ComputeKMeans SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<UInt64Parameter>(k_InitClusters_Key, "Number of Clusters", "This will be the tuple size for Cluster Attribute Matrix and the values within", 0));
  params.insert(
      std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(KUtilities::DistanceMetric::Euclidean),
                                         ChoicesParameter::Choices{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "DataPath to the boolean or uint8 mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Cluster", "The array to cluster from", DataPath{}, nx::core::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cluster Ids Array Name", "name of the ids array to be created in Attribute Array to Cluster's parent group",
                                                          "Cluster Ids"));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_FeatureAMPath_Key, "Cluster Attribute Matrix", "name and path of Attribute Matrix to hold Cluster Data", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MeansArrayName_Key, "Cluster Means Array Name", "name of the Means array to be created in Cluster Attribute Matrix", "Means"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeKMeansFilter::clone() const
{
  return std::make_unique<ComputeKMeansFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeKMeansFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto pInitClustersValue = filterArgs.value<uint64>(k_InitClusters_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto pFeatureAMPathValue = filterArgs.value<DataPath>(k_FeatureAMPath_Key);
  auto pMeansArrayNameValue = filterArgs.value<std::string>(k_MeansArrayName_Key);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto clusterArray = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);
  if(clusterArray == nullptr)
  {
    return MakePreflightErrorResult(-7585, "Array to Cluster MUST be a valid DataPath.");
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, clusterArray->getTupleShape(), std::vector<usize>{1}, pSelectedArrayPathValue.replaceName(pFeatureIdsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createAction));
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

  auto tupDims = std::vector<usize>{pInitClustersValue + 1};
  {
    auto createAction = std::make_unique<CreateAttributeMatrixAction>(pFeatureAMPathValue, tupDims);
    resultOutputActions.value().appendAction(std::move(createAction));
  }
  {
    auto createAction = std::make_unique<CreateArrayAction>(clusterArray->getDataType(), tupDims, clusterArray->getComponentShape(), pFeatureAMPathValue.createChildPath(pMeansArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeKMeansFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  if(!filterArgs.value<bool>(k_UseMask_Key))
  {
    maskPath = DataPath({k_MaskName});
    dataStructure.getDataRefAs<BoolArray>(maskPath).fill(true);
  }

  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  ComputeKMeansInputValues inputValues;

  inputValues.InitClusters = filterArgs.value<uint64>(k_InitClusters_Key);
  inputValues.DistanceMetric = static_cast<KUtilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.MaskArrayPath = maskPath;
  inputValues.MeansArrayPath = filterArgs.value<DataPath>(k_FeatureAMPath_Key).createChildPath(filterArgs.value<std::string>(k_MeansArrayName_Key));
  inputValues.Seed = seed;

  inputValues.ClusteringArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto fIdsPath = inputValues.ClusteringArrayPath.replaceName(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  dataStructure.getDataAs<Int32Array>(fIdsPath)->fill(0);
  inputValues.FeatureIdsArrayPath = fIdsPath;

  return ComputeKMeans(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InitClustersKey = "InitClusters";
constexpr StringLiteral k_DistanceMetricKey = "DistanceMetric";
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_UseRandomSeedKey = "UseRandomSeed";
constexpr StringLiteral k_RandomSeedValueKey = "RandomSeedValue";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
constexpr StringLiteral k_MeansArrayNameKey = "MeansArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeKMeansFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeKMeansFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_InitClustersKey, k_InitClusters_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DistanceMetricKey, k_DistanceMetric_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseRandomSeedKey, k_UseSeed_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64FilterParameterConverter>(args, json, SIMPL::k_RandomSeedValueKey, k_SeedValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(SIMPLConversion::Convert2Parameters<SIMPLConversion::AMPathBuilderFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, SIMPL::k_FeatureAttributeMatrixNameKey,
                                                                                                                k_FeatureAMPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_MeansArrayNameKey, k_MeansArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
