#include "KeepRemoveRankedFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/KeepRemoveRankedFeatures.hpp"

#include "simplnx/Common/DataTypeUtilities.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <cmath>
#include <functional>
#include <numeric>

using namespace nx::core;

namespace
{
constexpr int32 k_ParentNotAttributeMatrix = -78100;
constexpr int32 k_NotEnoughFeatures = -78101;
constexpr int32 k_PercentOutOfRange = -78102;
constexpr int32 k_ZeroNumFeatures = -78103;
constexpr int32 k_TupleCountMismatch = -78104;
constexpr int32 k_NumFeaturesClamped = -78120;
constexpr int32 k_PercentRoundedToZero = -78121;
constexpr int32 k_NothingFlagged = -78123;
constexpr int32 k_EverythingFlagged = -78124;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string KeepRemoveRankedFeaturesFilter::name() const
{
  return FilterTraits<KeepRemoveRankedFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string KeepRemoveRankedFeaturesFilter::className() const
{
  return FilterTraits<KeepRemoveRankedFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid KeepRemoveRankedFeaturesFilter::uuid() const
{
  return FilterTraits<KeepRemoveRankedFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string KeepRemoveRankedFeaturesFilter::humanName() const
{
  return "Keep/Remove Ranked Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> KeepRemoveRankedFeaturesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Threshold", "Rank", "Percentile", "Feature Removal"};
}

//------------------------------------------------------------------------------
Parameters KeepRemoveRankedFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether the ranked Features are the ones to [0] keep or the ones to [1] remove", to_underlying(RankOperation::Keep),
                                                   ChoicesParameter::Choices{"Keep Selected Features", "Remove Selected Features"}));
  params.insert(std::make_unique<ChoicesParameter>(k_RankFrom_Key, "Rank From", "Whether to rank from the [0] largest or [1] smallest value of the ranking array",
                                                   to_underlying(RankDirection::Largest), ChoicesParameter::Choices{"Largest", "Smallest"}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_Criterion_Key, "Selection Criterion", "How many Features to select: [0] an exact count, [1] a percentage of the Feature count, or [2] a percentage of the summed ranking value",
      to_underlying(RankCriterion::FeatureCount), ChoicesParameter::Choices{"Feature Count", "Percent of Feature Count", "Percent of Summed Value"}));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_NumFeatures_Key, "Number of Features", "The exact number of Features to select", 10ULL));
  params.insert(std::make_unique<NumberParameter<float64>>(k_Percent_Key, "Percent", "The percentage to select, in the range (0, 100]", 10.0));
  params.insert(std::make_unique<BoolParameter>(k_FillRemovedFeatures_Key, "Fill-in Removed Features", "Whether or not to fill in the gaps left by the removed Features", false));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RankingArrayPath_Key, "Feature Ranking Array", "The scalar Feature level array used to rank the Features", DataPath{},
                                                          nx::core::GetAllNumericTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when removing Features",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllDataTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_Criterion_Key, k_NumFeatures_Key, std::make_any<uint64>(to_underlying(RankCriterion::FeatureCount)));
  params.linkParameters(k_Criterion_Key, k_Percent_Key, std::make_any<uint64>(to_underlying(RankCriterion::PercentOfCount)));
  params.linkParameters(k_Criterion_Key, k_Percent_Key, std::make_any<uint64>(to_underlying(RankCriterion::PercentOfSum)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType KeepRemoveRankedFeaturesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer KeepRemoveRankedFeaturesFilter::clone() const
{
  return std::make_unique<KeepRemoveRankedFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult KeepRemoveRankedFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pRankingArrayPathValue = filterArgs.value<DataPath>(k_RankingArrayPath_Key);
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pCriterionValue = filterArgs.value<ChoicesParameter::ValueType>(k_Criterion_Key);
  auto pNumFeaturesValue = filterArgs.value<uint64>(k_NumFeatures_Key);
  auto pPercentValue = filterArgs.value<float64>(k_Percent_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const DataPath featureAttributeMatrixPath = pRankingArrayPathValue.getParent();
  const auto* featureAmPtr = dataStructure.getDataAs<AttributeMatrix>(featureAttributeMatrixPath);
  if(featureAmPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_ParentNotAttributeMatrix,
                                           fmt::format("The parent of the Feature Ranking Array at path '{}' must be an Attribute Matrix, but no Attribute Matrix was found at '{}'.",
                                                       pRankingArrayPathValue.toString(), featureAttributeMatrixPath.toString()))};
  }

  const auto& rankingArray = dataStructure.getDataRefAs<IDataArray>(pRankingArrayPathValue);
  const usize numTuples = rankingArray.getNumberOfTuples();
  if(numTuples < 2)
  {
    return {MakeErrorResult<OutputActions>(k_NotEnoughFeatures,
                                           fmt::format("The Feature Attribute Matrix at path '{}' holds {} tuple(s), but at least 2 are required. Tuple 0 is the unused dummy feature, so there "
                                                       "must be at least one real Feature to rank.",
                                                       featureAttributeMatrixPath.toString(), numTuples))};
  }
  const usize numFeatures = numTuples - 1;

  // validateNumberOfTuples() compares the supplied paths against one another, so passing the single
  // ranking array path would always succeed. Compare against the parent Attribute Matrix's shape
  // instead, which is the count the removal step actually relies on.
  const ShapeType& amShape = featureAmPtr->getShape();
  const usize amTupleCount = std::accumulate(amShape.cbegin(), amShape.cend(), static_cast<usize>(1), std::multiplies<>());
  if(numTuples != amTupleCount)
  {
    return {MakeErrorResult<OutputActions>(k_TupleCountMismatch, fmt::format("The Feature Ranking Array at path '{}' has {} tuples but its parent Attribute Matrix '{}' has {}. They must match.",
                                                                             pRankingArrayPathValue.toString(), numTuples, featureAttributeMatrixPath.toString(), amTupleCount))};
  }

  // Resolve the selected count wherever it is knowable without reading the data, so the user sees
  // the real cut at preflight rather than discovering it at execute.
  usize resolvedCount = 0;
  bool countIsResolvable = true;

  if(pCriterionValue == to_underlying(RankCriterion::FeatureCount))
  {
    if(pNumFeaturesValue == 0)
    {
      return {MakeErrorResult<OutputActions>(k_ZeroNumFeatures, "'Number of Features' is 0, which would select no Features. Enter a value of 1 or greater.")};
    }
    resolvedCount = static_cast<usize>(pNumFeaturesValue);
    if(resolvedCount > numFeatures)
    {
      resultOutputActions.m_Warnings.push_back(
          Warning{k_NumFeaturesClamped, fmt::format("'Number of Features' is {}, but only {} Features exist. Clamping the selection to {}.", pNumFeaturesValue, numFeatures, numFeatures)});
      resolvedCount = numFeatures;
    }
  }
  else
  {
    if(pPercentValue <= 0.0 || pPercentValue > 100.0)
    {
      return {MakeErrorResult<OutputActions>(k_PercentOutOfRange, fmt::format("'Percent' is {}, which is outside the allowed range (0, 100].", pPercentValue))};
    }
    if(pCriterionValue == to_underlying(RankCriterion::PercentOfCount))
    {
      resolvedCount = static_cast<usize>(std::llround(pPercentValue / 100.0 * static_cast<float64>(numFeatures)));
      if(resolvedCount == 0)
      {
        resultOutputActions.m_Warnings.push_back(Warning{k_PercentRoundedToZero, fmt::format("{}% of {} Features rounds to 0. Selecting 1 Feature instead.", pPercentValue, numFeatures)});
        resolvedCount = 1;
      }
    }
    else
    {
      // Percent of Summed Value depends on the actual values, so it cannot be resolved here.
      countIsResolvable = false;
    }
  }

  if(countIsResolvable)
  {
    // One uniform degenerate check covers every bad combination of operation, direction and criterion.
    const bool keep = (pOperationValue == to_underlying(RankOperation::Keep));
    const usize flaggedCount = keep ? numFeatures - resolvedCount : resolvedCount;
    if(flaggedCount == 0)
    {
      resultOutputActions.m_Warnings.push_back(
          Warning{k_NothingFlagged, fmt::format("The current settings leave all {} Features surviving, so no Features will be removed and this filter will do nothing.", numFeatures)});
    }
    else if(flaggedCount == numFeatures)
    {
      resultOutputActions.m_Warnings.push_back(
          Warning{k_EverythingFlagged, fmt::format("The current settings flag all {} Features for removal, which would leave nothing behind. This filter will fail during execute.", numFeatures)});
    }
  }

  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pFeatureIdsArrayPathValue.getParent(), {});
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pRankingArrayPathValue.getParent(), {});

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, pFeatureIdsArrayPathValue, pRankingArrayPathValue, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> KeepRemoveRankedFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  KeepRemoveRankedFeaturesInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.RankFrom = filterArgs.value<ChoicesParameter::ValueType>(k_RankFrom_Key);
  inputValues.Criterion = filterArgs.value<ChoicesParameter::ValueType>(k_Criterion_Key);
  inputValues.NumFeatures = filterArgs.value<uint64>(k_NumFeatures_Key);
  inputValues.Percent = filterArgs.value<float64>(k_Percent_Key);
  inputValues.FillRemovedFeatures = filterArgs.value<bool>(k_FillRemovedFeatures_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.RankingArrayPath = filterArgs.value<DataPath>(k_RankingArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return KeepRemoveRankedFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
