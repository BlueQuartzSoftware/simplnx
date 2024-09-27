#include "DBSCANFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/DBSCAN.hpp"

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
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace
{
const std::string k_MaskName = "temp_mask";
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string DBSCANFilter::name() const
{
  return FilterTraits<DBSCANFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string DBSCANFilter::className() const
{
  return FilterTraits<DBSCANFilter>::className;
}

//------------------------------------------------------------------------------
Uuid DBSCANFilter::uuid() const
{
  return FilterTraits<DBSCANFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string DBSCANFilter::humanName() const
{
  return "DBSCAN";
}

//------------------------------------------------------------------------------
std::vector<std::string> DBSCANFilter::defaultTags() const
{
  return {className(), "DBSCAN", "Clustering", "Segmentation", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters DBSCANFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitTypeIndex_Key, "Initialization Type",
                                                                    "Whether to use random or iterative for start state. See Documentation for further detail", to_underlying(AlgType::SeededRandom),
                                                                    ChoicesParameter::Choices{"Iterative", "Random", "Seeded Random"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "DBSCAN SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_UsePrecaching_Key, "Use Precaching", "If true the algorithm will be significantly faster, but it requires more memory", true));
  params.insert(std::make_unique<Float32Parameter>(k_Epsilon_Key, "Epsilon", "The epsilon-neighborhood around each point is queried", 0.0001));
  params.insert(std::make_unique<Int32Parameter>(k_MinPoints_Key, "Minimum Points",
                                                 "The minimum number of points needed to form a 'dense region' (i.e., the minimum number of points needed to be called a cluster)", 2));
  params.insert(
      std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(ClusterUtilities::DistanceMetric::Euclidean),
                                         ChoicesParameter::Choices{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array",
                                                          "DataPath to the boolean or uint8 mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Cluster", "The data array to cluster", DataPath{}, nx::core::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cluster Ids Array Name", "Name of the ids array to be created in Attribute Array to Cluster's parent group",
                                                          "Cluster Ids"));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_FeatureAMPath_Key, "Cluster Attribute Matrix", "The complete path to the attribute matrix in which to store to hold Cluster Data", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_InitTypeIndex_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(AlgType::Random)));
  params.linkParameters(k_InitTypeIndex_Key, k_SeedValue_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(AlgType::SeededRandom)));
  params.linkParameters(k_InitTypeIndex_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(AlgType::SeededRandom)));
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType DBSCANFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DBSCANFilter::clone() const
{
  return std::make_unique<DBSCANFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DBSCANFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto pFeatureAMPathValue = filterArgs.value<DataPath>(k_FeatureAMPath_Key);

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

  // Resized later
  {
    auto createAction = std::make_unique<CreateAttributeMatrixAction>(pFeatureAMPathValue, std::vector<usize>{1});
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // For caching seed run to run
  if(static_cast<AlgType>(filterArgs.value<ChoicesParameter::ValueType>(k_InitTypeIndex_Key)) != AlgType::Iterative)
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> DBSCANFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                   const std::atomic_bool& shouldCancel) const
{
  auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  if(!filterArgs.value<bool>(k_UseMask_Key))
  {
    maskPath = DataPath({k_MaskName});
    dataStructure.getDataRefAs<BoolArray>(maskPath).fill(true);
  }

  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(static_cast<AlgType>(filterArgs.value<ChoicesParameter::ValueType>(k_InitTypeIndex_Key)) != AlgType::SeededRandom)
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  if(static_cast<AlgType>(filterArgs.value<ChoicesParameter::ValueType>(k_InitTypeIndex_Key)) != AlgType::Iterative)
  {
    // Store Seed Value in Top Level Array
    dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;
  }

  DBSCANInputValues inputValues;

  inputValues.Epsilon = filterArgs.value<float32>(k_Epsilon_Key);
  inputValues.MinPoints = filterArgs.value<int32>(k_MinPoints_Key);
  inputValues.DistanceMetric = static_cast<ClusterUtilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.MaskArrayPath = maskPath;
  inputValues.ClusteringArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto fIdsPath = inputValues.ClusteringArrayPath.replaceName(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  dataStructure.getDataAs<Int32Array>(fIdsPath)->fill(0);
  inputValues.FeatureIdsArrayPath = fIdsPath;
  inputValues.FeatureAM = filterArgs.value<DataPath>(k_FeatureAMPath_Key);
  inputValues.AllowCaching = filterArgs.value<bool>(k_UsePrecaching_Key);
  inputValues.UseRandom = static_cast<AlgType>(filterArgs.value<ChoicesParameter::ValueType>(k_InitTypeIndex_Key)) != AlgType::Iterative;
  inputValues.Seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);

  return DBSCAN(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_EpsilonKey = "Epsilon";
constexpr StringLiteral k_MinPointsKey = "MinPnts";
constexpr StringLiteral k_DistanceMetricKey = "DistanceMetric";
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> DBSCANFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = DBSCANFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_EpsilonKey, k_Epsilon_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_MinPointsKey, k_MinPoints_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DistanceMetricKey, k_DistanceMetric_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(SIMPLConversion::Convert2Parameters<SIMPLConversion::AMPathBuilderFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, SIMPL::k_FeatureAttributeMatrixNameKey,
                                                                                                                k_FeatureAMPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
