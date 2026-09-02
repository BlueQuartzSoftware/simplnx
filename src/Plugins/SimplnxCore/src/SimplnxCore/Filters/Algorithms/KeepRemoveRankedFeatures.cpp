#include "KeepRemoveRankedFeatures.hpp"

#include "SimplnxCore/utils/FeatureRemovalUtilities.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_NotEnoughFeatures = -78101;
constexpr int32 k_NonFiniteValue = -78105;
constexpr int32 k_NegativeValue = -78106;
constexpr int32 k_ZeroTotalSum = -78107;
constexpr int32 k_AllFeaturesFlagged = -78108;
constexpr int32 k_TieStraddlesCutoff = -78122;

// How often the feature level loops poll for cancellation. Frequent enough to stay responsive on
// millions of features, rare enough not to slow the loops down.
constexpr usize k_CancelCheckStride = 65536;

/**
 * @brief Sorts the feature indices by their ranking value and builds the removal flag vector.
 *
 * The comparator orders by value first and then by ascending feature id. That total order makes the
 * result independent of the sort algorithm's internals, so repeated runs agree across platforms.
 */
struct RankAndFlagFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& rankingArray, const KeepRemoveRankedFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler,
                      std::vector<bool>& flags, std::string& summary, std::vector<Warning>& warnings)
  {
    const auto& rankingRef = dynamic_cast<const DataArray<T>&>(rankingArray).getDataStoreRef();
    const usize numTuples = rankingRef.getNumberOfTuples();

    // The filter's preflight already rejects this, but the algorithm is exported and can be
    // constructed directly. Without the guard numTuples == 0 underflows below and numTuples == 1
    // leaves an empty sort order that the selection loop would index into.
    if(numTuples < 2)
    {
      return MakeErrorResult(k_NotEnoughFeatures, fmt::format("The ranking array '{}' holds {} tuple(s), but at least 2 are required. Tuple 0 is the unused dummy feature, so there must be at "
                                                              "least one real Feature to rank.",
                                                              rankingArray.getName(), numTuples));
    }
    const usize numFeatures = numTuples - 1;

    // Three feature sized buffers are live at once from here on: values (numFeatures * sizeof(T)),
    // order (numFeatures * sizeof(usize)) and flags (numFeatures bits). For 10 million features with
    // an int32 ranking array that totals roughly 120 MB. No cell sized buffer is ever allocated.
    //
    // Sorting reads the values in random order, which is close to the worst case for a chunked
    // out-of-core store. Pull them into a contiguous buffer with one sequential pass first, so each
    // chunk is faulted in once and fully consumed. Tuple 0 is the unused dummy feature and is
    // excluded, so values[i] belongs to feature id i + 1.
    messageHandler.sendInfoMessage(fmt::format("Reading {} ranking values...", numFeatures));
    std::vector<T> values(numFeatures);
    for(usize i = 0; i < numFeatures; i++)
    {
      if(i % k_CancelCheckStride == 0 && shouldCancel)
      {
        return {};
      }
      values[i] = rankingRef[i + 1];
    }

    // std::sort with a comparator that sees NaN violates strict weak ordering. That is undefined
    // behavior and segfaults in practice, so reject non-finite values before sorting, not after.
    if constexpr(std::is_floating_point_v<T>)
    {
      for(usize i = 0; i < numFeatures; i++)
      {
        if(i % k_CancelCheckStride == 0 && shouldCancel)
        {
          return {};
        }
        if(!std::isfinite(static_cast<float64>(values[i])))
        {
          return MakeErrorResult(k_NonFiniteValue, fmt::format("Feature {} in the ranking array '{}' holds the non-finite value {}. Features cannot be ranked against a value that has no "
                                                               "ordering. Remove or replace the non-finite values before running this filter.",
                                                               i + 1, rankingArray.getName(), values[i]));
        }
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    // std::sort cannot be interrupted, so this is the last cancel opportunity before it runs.
    messageHandler.sendInfoMessage(fmt::format("Sorting {} Features by rank...", numFeatures));
    std::vector<usize> order(numFeatures);
    std::iota(order.begin(), order.end(), 0);

    const bool descending = (inputValues->RankFrom == to_underlying(RankDirection::Largest));
    std::sort(order.begin(), order.end(), [&values, descending](usize lhs, usize rhs) {
      if(values[lhs] != values[rhs])
      {
        return descending ? values[lhs] > values[rhs] : values[lhs] < values[rhs];
      }
      return lhs < rhs;
    });

    usize selectedCount = 0;
    if(inputValues->Criterion == to_underlying(RankCriterion::FeatureCount))
    {
      selectedCount = std::min(static_cast<usize>(inputValues->NumFeatures), numFeatures);
    }
    else if(inputValues->Criterion == to_underlying(RankCriterion::PercentOfCount))
    {
      selectedCount = static_cast<usize>(std::llround(inputValues->Percent / 100.0 * static_cast<float64>(numFeatures)));
      // Never round down to selecting nothing; a small percentage still means "a few".
      selectedCount = std::max(static_cast<usize>(1), std::min(selectedCount, numFeatures));
    }
    else
    {
      // Percent of Summed Value. Only meaningful for extensive quantities such as NumElements or
      // Volumes; a cumulative fraction of an aspect ratio has no physical interpretation.
      float64 total = 0.0;
      for(usize i = 0; i < numFeatures; i++)
      {
        if(i % k_CancelCheckStride == 0 && shouldCancel)
        {
          return {};
        }
        if constexpr(std::is_signed_v<T>)
        {
          if(values[i] < static_cast<T>(0))
          {
            return MakeErrorResult(k_NegativeValue, fmt::format("The 'Percent of Summed Value' criterion requires non-negative values, but Feature {} in the array '{}' holds the value {}. A "
                                                                "cumulative fraction is undefined when values can cancel each other out.",
                                                                i + 1, rankingArray.getName(), values[i]));
          }
        }
        total += static_cast<float64>(values[i]);
      }

      if(total <= 0.0)
      {
        return MakeErrorResult(k_ZeroTotalSum, fmt::format("The 'Percent of Summed Value' criterion requires a positive total, but the {} values in the array '{}' sum to {}.", numFeatures,
                                                           rankingArray.getName(), total));
      }

      // Accumulate in rank order and include the feature that crosses the target, so any percentage
      // above zero selects at least one feature and the selection always reaches the target.
      const float64 target = inputValues->Percent / 100.0 * total;
      float64 runningSum = 0.0;
      while(selectedCount < numFeatures && runningSum < target)
      {
        runningSum += static_cast<float64>(values[order[selectedCount]]);
        selectedCount++;
      }
    }

    // Ties are broken by ascending feature id, so exactly selectedCount features are always chosen.
    // That choice is arbitrary among equals, and ties are common in integer arrays like NumElements,
    // so say so rather than letting the user assume the cut was meaningful.
    if(selectedCount > 0 && selectedCount < numFeatures && values[order[selectedCount - 1]] == values[order[selectedCount]])
    {
      const T tiedValue = values[order[selectedCount - 1]];
      const auto tiedTotal = static_cast<usize>(std::count(values.cbegin(), values.cend(), tiedValue));
      warnings.push_back(Warning{k_TieStraddlesCutoff, fmt::format("A tie straddles the cutoff: {} Features share the value {} at the boundary of the {} selected. Ties are broken by ascending "
                                                                   "feature id, so exactly {} Features were selected and the choice among the tied Features is arbitrary.",
                                                                   tiedTotal, tiedValue, selectedCount, selectedCount)});
    }

    // 'true' means flagged for removal. Under Keep, the selected features survive and everything
    // else is flagged; under Remove it is the other way around. Both fall out of the polarity here,
    // so there is no per-combination branching.
    const bool keep = (inputValues->Operation == to_underlying(RankOperation::Keep));
    flags.assign(numTuples, keep);
    for(usize i = 0; i < selectedCount; i++)
    {
      flags[order[i] + 1] = !keep;
    }
    flags[0] = false;

    const auto flaggedCount = static_cast<usize>(std::count(flags.cbegin() + 1, flags.cend(), true));
    if(flaggedCount == numFeatures)
    {
      return MakeErrorResult(k_AllFeaturesFlagged, fmt::format("{} the {} {} of {} features would flag every feature for removal, leaving nothing behind. Adjust the selection so at least one "
                                                               "feature survives.",
                                                               keep ? "Keeping" : "Removing", selectedCount, descending ? "largest" : "smallest", numFeatures));
    }

    summary = fmt::format("{} the {} {} features by '{}'; flagged {} of {} features for removal.", keep ? "Keeping" : "Removing", selectedCount, descending ? "largest" : "smallest",
                          rankingArray.getName(), flaggedCount, numFeatures);
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
KeepRemoveRankedFeatures::KeepRemoveRankedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   KeepRemoveRankedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
KeepRemoveRankedFeatures::~KeepRemoveRankedFeatures() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& KeepRemoveRankedFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> KeepRemoveRankedFeatures::operator()()
{
  const auto& rankingArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->RankingArrayPath);

  std::vector<bool> flags;
  std::string summary;
  std::vector<Warning> warnings;

  Result<> selectionResult = ExecuteDataFunction(RankAndFlagFunctor{}, rankingArray.getDataType(), rankingArray, m_InputValues, m_ShouldCancel, m_MessageHandler, flags, summary, warnings);

  // Carry any warnings out on the failure path too. A tie warning is raised before the all flagged
  // check, and dropping it would hide the most useful diagnostic behind the error.
  selectionResult.warnings().insert(selectionResult.warnings().end(), warnings.cbegin(), warnings.cend());
  if(selectionResult.invalid())
  {
    return selectionResult;
  }

  if(getCancel())
  {
    return {};
  }

  // Report the user's intent rather than the raw polarity. A message that says only "490 features
  // flagged true" is the kind that gets confirmed without actually being checked.
  m_MessageHandler.sendInfoMessage(summary);

  FeatureRemovalUtilities::RemovalArgs removalArgs;
  removalArgs.ImageGeometryPath = m_InputValues->ImageGeometryPath;
  removalArgs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
  removalArgs.FeatureAttributeMatrixPath = m_InputValues->RankingArrayPath.getParent();
  removalArgs.IgnoredDataArrayPaths = m_InputValues->IgnoredDataArrayPaths;
  removalArgs.FillRemovedFeatures = m_InputValues->FillRemovedFeatures;

  Result<> result = FeatureRemovalUtilities::removeFlaggedFeatures(m_DataStructure, flags, removalArgs, m_MessageHandler, m_ShouldCancel);
  result.warnings().insert(result.warnings().end(), warnings.cbegin(), warnings.cend());
  return result;
}
