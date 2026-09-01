#include "ComputeFeaturePhases.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_MaxListedFeatures = 15;

/// Number of tuples to read per bulk I/O call. 64K tuples (256 KB of int32 per
/// buffer) minimizes copyIntoBuffer() round-trips on large datasets while keeping
/// the per-chunk working set bounded regardless of total cell count.
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeFeaturePhases::ComputeFeaturePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeaturePhases::~ComputeFeaturePhases() noexcept = default;

Result<> ComputeFeaturePhases::operator()()
{
  auto featurePhasesArrayPath = m_InputValues->CellFeaturesAttributeMatrixPath.createChildPath(m_InputValues->FeaturePhasesArrayName);

  const auto& cellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath)->getDataStoreRef();
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(featurePhasesArrayPath)->getDataStoreRef();

  // Validation makes every Feature ID valid for the feature-sized caches.
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CellFeaturesAttributeMatrixPath, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize numFeatures = featurePhases.getNumberOfTuples();
  // Feature-sized caches avoid DataStore access in the cell loop. uint8 avoids vector<bool> proxy access.
  std::vector<int32> featurePhaseValues(numFeatures, 0);
  std::vector<uint8> featureSeen(numFeatures, 0);
  std::set<int32> warnFeatures;

  // Matching chunks amortize input I/O and preserve serial last-phase order.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto cellPhaseBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    Result<> readFidResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    if(readFidResult.invalid())
    {
      return readFidResult;
    }
    Result<> readPhaseResult = cellPhases.copyIntoBuffer(offset, nonstd::span<int32>(cellPhaseBuf.get(), count));
    if(readPhaseResult.invalid())
    {
      return readPhaseResult;
    }

    for(usize i = 0; i < count; i++)
    {
      const int32 featureId = featureIdBuf[i];
      const int32 phase = cellPhaseBuf[i];

      if(featureId == 0)
      {
        continue;
      }
      if(phase < 0)
      {
        return MakeErrorResult(-61861, fmt::format("Cell phases contains a negative value. Index: {} | Value: {}", offset + i, phase));
      }

      // A feature warns when any of its cells carries a phase different from the
      // first phase seen for that feature. Comparing each cell against the
      // PREVIOUS phase stored for the feature detects exactly the same feature
      // set: over the feature's cell sequence, "some element differs from the
      // first" holds if and only if "some adjacent pair differs". Storing the
      // phase unconditionally afterwards means the feature keeps the LAST phase
      // encountered, which is also the value written to the output below.
      if(featureSeen[featureId] != 0 && featurePhaseValues[featureId] != phase)
      {
        warnFeatures.insert(featureId);
      }
      featurePhaseValues[featureId] = phase;
      featureSeen[featureId] = 1;
    }
  }

  // One bulk write avoids scattered feature output writes.
  Result<> writeResult = featurePhases.copyFromBuffer(0, nonstd::span<const int32>(featurePhaseValues.data(), numFeatures));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  Result<> result;
  if(!warnFeatures.empty())
  {
    std::string warnStr = "Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used. Affected Phase Features: ";
    usize position = 0;
    for(auto value : warnFeatures)
    {
      if(position < k_MaxListedFeatures)
      {
        if(position > 0)
        {
          warnStr.append(", ");
        }
        warnStr.append(std::to_string(value));
      }
      position++;
    }
    if(position > k_MaxListedFeatures)
    {
      const usize remainder = position - k_MaxListedFeatures;
      warnStr.append(fmt::format(", and {} more {}", remainder, remainder == 1 ? "occurrence" : "occurrences"));
    }
    result.warnings().push_back(Warning{-500, std::move(warnStr)});
  }

  return result;
}
