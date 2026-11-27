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
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>
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

  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_ParseOrderIndex_Key, "Parse Order", "Whether to use random or low density first for parse order. See Documentation for further detail", to_underlying(DBSCAN::ParseOrder::LowDensityFirst),
      ChoicesParameter::ChoicesType{"Low Density First", "Random", "Seeded Random"})); // Choice order matches DBSCAN::ParseOrder values.
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "DBSCAN SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32ParameterType>(
      k_Epsilon_Key, "Epsilon", "The epsilon-neighborhood around each point is queried (i.e., the maximum acceptable distance between points to be considered `connected`)", 0.0001f));
  params.insert(std::make_unique<Int32ParameterType>(k_MinPoints_Key, "Minimum Points",
                                                 "The minimum number of points needed to form a 'dense region' (i.e., the minimum number of points needed to be called a cluster)", 2));
  params.insert(std::make_unique<ChoicesParameter>(
      k_DistanceMetric_Key, "Distance Metric", "Distance Metric type to be used for calculations", to_underlying(cluster_utilities::DistanceMetric::Euclidean),
      ChoicesParameter::ChoicesType{"Euclidean", "Squared Euclidean", "Manhattan", "Cosine", "Pearson", "Squared Pearson"})); // Choice order matches ClusterUtilities::DistanceMetric values.

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array",
                                                          "DataPath to the boolean or uint8 mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Cluster", "The data array to cluster", DataPath{}, nx::core::GetAllNumericTypes(),
                                                          ArraySelectionParameter::AllowedComponentShapesType{{2}, {3}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Cluster Ids Array Name", "Name of the ids array to be created in Attribute Array to Cluster's parent group",
                                                          "Cluster Ids"));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_FeatureAMPath_Key, "Cluster Attribute Matrix", "The complete path to the attribute matrix in which to store to hold Cluster Data", DataPath{}));

  params.linkParameters(k_ParseOrderIndex_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::Random)));
  params.linkParameters(k_ParseOrderIndex_Key, k_SeedValue_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::SeededRandom)));
  params.linkParameters(k_ParseOrderIndex_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::SeededRandom)));
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType DBSCANFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Change 1:
  // Removed - k_UsePrecaching_Key = "use_precaching";
  // Solution - Ignore Variable
  //
  // Change 2:
  // Modified Existing - k_ParseOrderIndex_Key = "init_type_index" -> k_ParseOrderIndex_Key = "parse_order_index";
  // Solution - Read the value of old parameter into new;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType DBSCANFilter::clone() const
{
  return std::make_unique<DBSCANFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DBSCANFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  const auto pEpsilonValue = filterArgs.value<float32>(k_Epsilon_Key);
  const auto pMinPointsValue = filterArgs.value<int32>(k_MinPoints_Key);
  const auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  const auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  const auto pFeatureAMPathValue = filterArgs.value<DataPath>(k_FeatureAMPath_Key);

  if(!std::isfinite(pEpsilonValue) || pEpsilonValue <= 0.0f)
  {
    return MakePreflightErrorResult(-7584, fmt::format("Epsilon value {} must be a finite number greater than 0.", pEpsilonValue));
  }
  if(pMinPointsValue <= 0)
  {
    return MakePreflightErrorResult(-7585, fmt::format("Minimum Points value {} must be greater than 0.", pMinPointsValue));
  }

  Result<OutputActions> resultOutputActions;

  const auto& clusterArray = dataStructure.getDataRefAs<IDataArray>(pSelectedArrayPathValue);

  if(filterArgs.value<bool>(k_UseMask_Key))
  {
    const auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
    if(!CheckArraysHaveSameTupleCount(dataStructure, {pSelectedArrayPathValue, maskPath}))
    {
      const auto& maskArray = dataStructure.getDataRefAs<IArray>(maskPath);
      return MakePreflightErrorResult(
          -7587, fmt::format("**Attribute Array to Cluster** DataArray '{}' has {} tuples, but **Cell Mask Array** DataArray '{}' has {} tuples. Select DataArrays with the same number of tuples.",
                             pSelectedArrayPathValue.toString(), clusterArray.getNumberOfTuples(), maskPath.toString(), maskArray.getNumberOfTuples()));
    }
  }

  {
    auto createAction =
        std::make_unique<CreateArrayAction>(DataType::int32, clusterArray.getTupleShape(), std::vector<usize>{1}, pSelectedArrayPathValue.replaceName(pFeatureIdsArrayNameValue), "", "0");
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // The algorithm resizes the cluster AttributeMatrix after it determines the cluster count.
  {
    auto createAction = std::make_unique<CreateAttributeMatrixAction>(pFeatureAMPathValue, std::vector<usize>{1});
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Randomized parse-order modes create a top-level seed output.
  if(static_cast<DBSCAN::ParseOrder>(filterArgs.value<ChoicesParameter::ValueType>(k_ParseOrderIndex_Key)) != DBSCAN::ParseOrder::LowDensityFirst)
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> DBSCANFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  const bool useMask = filterArgs.value<bool>(k_UseMask_Key);
  const auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(static_cast<DBSCAN::ParseOrder>(filterArgs.value<ChoicesParameter::ValueType>(k_ParseOrderIndex_Key)) != DBSCAN::ParseOrder::SeededRandom)
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  if(static_cast<DBSCAN::ParseOrder>(filterArgs.value<ChoicesParameter::ValueType>(k_ParseOrderIndex_Key)) != DBSCAN::ParseOrder::LowDensityFirst)
  {
    // The seed output records the selected or generated seed value.
    dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;
  }

  DBSCANInputValues inputValues;

  inputValues.epsilon = filterArgs.value<float32>(k_Epsilon_Key);
  inputValues.min_points = filterArgs.value<int32>(k_MinPoints_Key);
  inputValues.distance_metric = static_cast<cluster_utilities::DistanceMetric>(filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key));
  inputValues.use_mask = useMask;
  inputValues.mask_array_path = maskPath;
  inputValues.clustering_array_path = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.feature_ids_array_path = inputValues.clustering_array_path.replaceName(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  inputValues.feature_am = filterArgs.value<DataPath>(k_FeatureAMPath_Key);
  inputValues.parse_order = filterArgs.value<ChoicesParameter::ValueType>(k_ParseOrderIndex_Key);
  inputValues.seed = seed;

  return DBSCAN(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_EpsilonKey = "Epsilon";
constexpr StringLiteralType k_MinPointsKey = "MinPnts";
constexpr StringLiteralType k_DistanceMetricKey = "DistanceMetric";
constexpr StringLiteralType k_UseMaskKey = "UseMask";
constexpr StringLiteralType k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteralType k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteralType k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
} // namespace simpl
} // namespace

Result<Arguments> DBSCANFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = DBSCANFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_EpsilonKey, k_Epsilon_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_MinPointsKey, k_MinPoints_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DistanceMetricKey, k_DistanceMetric_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseMaskKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedArrayPathKey, k_SelectedArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(simpl_conversion::Convert2Parameters<simpl_conversion::AMPathBuilderFilterParameterConverter>(args, json, simpl::k_SelectedArrayPathKey, simpl::k_FeatureAttributeMatrixNameKey,
                                                                                                                k_FeatureAMPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
