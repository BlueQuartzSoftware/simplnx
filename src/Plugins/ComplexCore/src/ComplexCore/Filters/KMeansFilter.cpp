#include "KMeansFilter.hpp"

#include "ComplexCore/Filters/Algorithms/KMeans.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/KUtilities.hpp"

#include <random>

using namespace complex;

namespace
{
const std::string k_MaskName = "temp_mask";
}

namespace complex
{
//------------------------------------------------------------------------------
std::string KMeansFilter::name() const
{
  return FilterTraits<KMeansFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string KMeansFilter::className() const
{
  return FilterTraits<KMeansFilter>::className;
}

//------------------------------------------------------------------------------
Uuid KMeansFilter::uuid() const
{
  return FilterTraits<KMeansFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string KMeansFilter::humanName() const
{
  return "K Means";
}

//------------------------------------------------------------------------------
std::vector<std::string> KMeansFilter::defaultTags() const
{
  return {className(), "DREAM3D Review", "Clustering"};
}

//------------------------------------------------------------------------------
Parameters KMeansFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<UInt64Parameter>(k_SeedValue_Key, "Seed", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}));

  params.insert(std::make_unique<UInt64Parameter>(k_InitClusters_Key, "Number of Clusters", "This will be the tuple size for Cluster Attribute Matrix and the values within", 0));
  params.insert(
      std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(KUtilities::DistanceMetric::Euclidean),
                                         ChoicesParameter::Choices{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Cluster", "The array to cluster from", DataPath{}, complex::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
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
IFilter::UniquePointer KMeansFilter::clone() const
{
  return std::make_unique<KMeansFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult KMeansFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto pInitClustersValue = filterArgs.value<uint64>(k_InitClusters_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto pFeatureAMPathValue = filterArgs.value<DataPath>(k_FeatureAMPath_Key);
  auto pMeansArrayNameValue = filterArgs.value<std::string>(k_MeansArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto clusterArray = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);
  if(clusterArray == nullptr)
  {
    return MakePreflightErrorResult(-7585, "Array to Cluster MUST be a valid DataPath.");
  }

  {
    auto createAction =
        std::make_unique<CreateArrayAction>(DataType::int32, clusterArray->getTupleShape(), std::vector<usize>{1}, pSelectedArrayPathValue.getParent().createChildPath(pFeatureIdsArrayNameValue));
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

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> KMeansFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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

  KMeansInputValues inputValues;

  inputValues.InitClusters = filterArgs.value<uint64>(k_InitClusters_Key);
  inputValues.DistanceMetric = static_cast<KUtilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.MaskArrayPath = maskPath;
  inputValues.MeansArrayPath = filterArgs.value<DataPath>(k_FeatureAMPath_Key).createChildPath(filterArgs.value<std::string>(k_MeansArrayName_Key));
  inputValues.Seed = seed;

  inputValues.ClusteringArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto fIdsPath = inputValues.ClusteringArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  dataStructure.getDataAs<Int32Array>(fIdsPath)->fill(0);
  inputValues.FeatureIdsArrayPath = fIdsPath;

  return KMeans(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
