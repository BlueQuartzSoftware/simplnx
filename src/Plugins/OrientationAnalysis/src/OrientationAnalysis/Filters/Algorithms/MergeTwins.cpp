#include "MergeTwins.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
MergeTwins::MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
}

// -----------------------------------------------------------------------------
MergeTwins::~MergeTwins() noexcept = default;

// -----------------------------------------------------------------------------
Result<int32> MergeTwins::getSeed(int32 newFid)
{
  auto& phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  auto& cellFeaturesAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixPath);

  auto numFeatures = static_cast<int32>(phases.getNumberOfTuples());

  int32 seed = -1;

  // Precalculate some constants
  int32 totalFMinus1 = numFeatures - 1;

  usize counter = 0;

  auto randFeature = static_cast<int32>(m_Distribution(m_Generator) * static_cast<float32>(totalFMinus1));

  while(seed == -1 && counter < numFeatures)
  {
    if(randFeature > totalFMinus1)
    {
      randFeature = randFeature - numFeatures;
    }
    if(featureParentIds[randFeature] == -1)
    {
      seed = randFeature;
    }
    randFeature++;
    counter++;
  }
  if(seed >= 0)
  {
    featureParentIds[seed] = newFid;
    ShapeType tDims = {newFid + 1ULL};
    if(Result<> resizeResult = cellFeaturesAttMatrix.resizeTuples(tDims); resizeResult.invalid())
    {
      return ConvertInvalidResult<int32>(std::move(resizeResult));
    }
  }
  return {seed};
}

// -----------------------------------------------------------------------------
Result<bool> MergeTwins::determineGrouping(int32 referenceFeatureIdx, int32 neighborFeatureIdx, int32 newFid)
{
  const auto& featurePhasesStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  auto& featureParentIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  const auto& avgQuatsStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath)->getDataStoreRef();
  const float32 axisToleranceRad = m_InputValues->AxisTolerance * numbers::pi_v<float32> / 180.0F;

  bool twin = false;

  const int32 currentPhaseIdx = featurePhasesStoreRef[referenceFeatureIdx];
  const int32 neighborFeaturePhaseIdx = featurePhasesStoreRef[neighborFeatureIdx];
  if(featureParentIdsStoreRef[neighborFeatureIdx] == -1 && currentPhaseIdx > 0 && neighborFeaturePhaseIdx > 0)
  {
    const usize numCrystalStructures = crystalStructuresStoreRef.getNumberOfTuples();
    if(static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
    {
      return MakeErrorResult<bool>(
          -23502,
          fmt::format("Feature Phases array '{}' has value {} at reference Feature index {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                      m_InputValues->FeaturePhasesArrayPath.toString(), currentPhaseIdx, referenceFeatureIdx, m_InputValues->CrystalStructuresArrayPath.toString(), numCrystalStructures,
                      numCrystalStructures));
    }
    if(static_cast<usize>(neighborFeaturePhaseIdx) >= numCrystalStructures)
    {
      return MakeErrorResult<bool>(
          -23502, fmt::format("Feature Phases array '{}' has value {} at neighbor Feature index {}, but Crystal Structures array '{}' contains {} tuples. Valid positive Phase indices are in [1, {}).",
                              m_InputValues->FeaturePhasesArrayPath.toString(), neighborFeaturePhaseIdx, neighborFeatureIdx, m_InputValues->CrystalStructuresArrayPath.toString(), numCrystalStructures,
                              numCrystalStructures));
    }

    const uint32 currentLaueIndex = crystalStructuresStoreRef[currentPhaseIdx];

    const ebsdlib::QuatD q1(avgQuatsStoreRef[referenceFeatureIdx * 4], avgQuatsStoreRef[referenceFeatureIdx * 4 + 1], avgQuatsStoreRef[referenceFeatureIdx * 4 + 2],
                            avgQuatsStoreRef[referenceFeatureIdx * 4 + 3]);
    const ebsdlib::QuatD q2(avgQuatsStoreRef[neighborFeatureIdx * 4], avgQuatsStoreRef[neighborFeatureIdx * 4 + 1], avgQuatsStoreRef[neighborFeatureIdx * 4 + 2],
                            avgQuatsStoreRef[neighborFeatureIdx * 4 + 3]);

    const uint32 neighborLaueIndex = crystalStructuresStoreRef[neighborFeaturePhaseIdx];
    if(currentLaueIndex == neighborLaueIndex && currentLaueIndex == ebsdlib::CrystalStructure::Cubic_High)
    {
      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[currentLaueIndex]->calculateMisorientation(q1, q2);
      double w = axisAngle[3];
      w *= (180.0f / numbers::pi);
      double axisDiff111 = std::acos(std::fabs(axisAngle[0]) * 0.57735f + std::fabs(axisAngle[1]) * 0.57735f + fabs(axisAngle[2]) * 0.57735f);
      double angDiff60 = std::fabs(w - 60.0f);
      if(axisDiff111 < axisToleranceRad && angDiff60 < m_InputValues->AngleTolerance)
      {
        twin = true;
      }
      if(twin)
      {
        featureParentIdsStoreRef[neighborFeatureIdx] = newFid;
        return {true};
      }
    }
  }
  return {false};
}

Result<> MergeTwins::groupFeaturesExecute()
{
  auto& conNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->ContiguousNeighborListArrayPath);
  std::vector<int32_t> groupList;

  int32_t parentCount = 0;
  int32_t featureSeed = 0;
  int32_t list1size = 0, list2size = 0, listsize = 0;
  int32 neighborFeatureIdx = 0;

  while(featureSeed >= 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    bool m_PatchGrouping = false;
    parentCount++;
    Result<int32> seedResult = getSeed(parentCount);
    if(seedResult.invalid())
    {
      return ConvertResult(std::move(seedResult));
    }
    featureSeed = seedResult.value();
    if(featureSeed >= 0)
    {
      groupList.push_back(featureSeed);
      for(std::vector<int32_t>::size_type groupListIdx = 0; groupListIdx < groupList.size(); groupListIdx++)
      {
        const int32 currentFeatureIdx = groupList[groupListIdx];
        list1size = int32_t(conNeighborList[currentFeatureIdx].size());

        for(int32 neighborListIdx = 0; neighborListIdx < 2; neighborListIdx++)
        {
          if(neighborListIdx == 0)
          {
            listsize = list1size;
          }
          else if(neighborListIdx == 1)
          {
            listsize = list2size;
          }
          for(int32 neighborIdx = 0; neighborIdx < listsize; neighborIdx++)
          {
            if(neighborListIdx == 0)
            {
              neighborFeatureIdx = conNeighborList[currentFeatureIdx][neighborIdx];
            }
            else if(neighborListIdx == 1)
            {
            }
            if(neighborFeatureIdx != currentFeatureIdx)
            {
              Result<bool> groupingResult = determineGrouping(currentFeatureIdx, neighborFeatureIdx, parentCount);
              if(groupingResult.invalid())
              {
                return ConvertResult(std::move(groupingResult));
              }
              if(groupingResult.value())
              {
                if(!m_PatchGrouping)
                {
                  groupList.push_back(neighborFeatureIdx);
                }
              }
            }
          }
        }
      }
    }
    groupList.clear();
  }
  return {};
}

Result<> MergeTwins::operator()()
{
  Result result = {};

  m_Generator = std::mt19937_64(std::mt19937::default_seed);
  m_Distribution = std::uniform_real_distribution<float32>(0.0f, 1.0f);

  /* Sanity check that each phase is Cubic High (m3m) Laue class. If not then warn the user.
   * There is code later on to ensure that only m3m Laue class is used.
   */
  auto& laueClasses = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& cellParentIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellParentIdsArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();

  usize totalPoints = cellParentIdsStore.getNumberOfTuples();

  // Initialize cellParentIds to -1 using chunked bulk writes. For OOC stores,
  // a single fill() call would trigger per-element virtual dispatch; chunked
  // copyFromBuffer amortizes the overhead over 64K-tuple writes.
  {
    constexpr usize k_FillChunk = 65536;
    std::vector<int32> fillBuf(k_FillChunk, -1);
    for(usize offset = 0; offset < totalPoints; offset += k_FillChunk)
    {
      usize count = std::min(k_FillChunk, totalPoints - offset);
      if(Result<> ioResult = cellParentIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(fillBuf.data(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
    }
  }

  featureParentIds.fill(-1);

  for(usize i = 1; i < laueClasses.getSize(); i++)
  {
    if(laueClasses[i] != ebsdlib::CrystalStructure::Cubic_High)
    {
      std::string msg = fmt::format("Phase '{}' is NOT m3m crystal symmetry. Data from this phase will not be used in this filter.", i);
      result = MakeWarningVoidResult(-23500, msg);
    }
  }
  auto mergeWarnings = [&result](Result<> operationResult) { return MergeResults(std::move(result), std::move(operationResult)); };

  featureParentIds[0] = 0; // set feature 0 to be parent 0

  if(Result<> groupingResult = groupFeaturesExecute(); groupingResult.invalid())
  {
    return mergeWarnings(std::move(groupingResult));
  }

  auto& active = m_DataStructure.getDataAs<BoolArray>(m_InputValues->ActiveArrayPath)->getDataStoreRef();
  active.fill(true);

  usize totalFeatures = active.getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return mergeWarnings(
        ConvertResult(MakeErrorResult<OutputActions>(-23501, "The number of grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high")));
  }

  // The local feature-parent cache avoids random OOC lookup in the cell loop.
  const usize numFeatures = featureParentIds.getNumberOfTuples();
  std::vector<int32> featureParentIdsCache(numFeatures);
  if(Result<> ioResult = featureParentIds.copyIntoBuffer(0, nonstd::span<int32>(featureParentIdsCache.data(), numFeatures)); ioResult.invalid())
  {
    return mergeWarnings(std::move(ioResult));
  }

  // Chunked cells use the local feature-parent cache.
  int32 numParents = 0;
  {
    constexpr usize k_ChunkSize = 65536;
    std::vector<int32> featureIdsBuf(k_ChunkSize);
    std::vector<int32> cellParentIdsBuf(k_ChunkSize);

    for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      usize count = std::min(k_ChunkSize, totalPoints - offset);
      if(Result<> ioResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdsBuf.data(), count)); ioResult.invalid())
      {
        return mergeWarnings(std::move(ioResult));
      }

      for(usize i = 0; i < count; i++)
      {
        int32 featureName = featureIdsBuf[i];
        cellParentIdsBuf[i] = featureParentIdsCache[featureName];
        if(featureParentIdsCache[featureName] > numParents)
        {
          numParents = featureParentIdsCache[featureName];
        }
      }

      if(Result<> ioResult = cellParentIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(cellParentIdsBuf.data(), count)); ioResult.invalid())
      {
        return mergeWarnings(std::move(ioResult));
      }
    }
  }
  numParents += 1;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  if(m_InputValues->RandomizeParentIds)
  { // Randomize Parent IDs
    m_MessageHandler({IFilter::Message::Type::Info, "Randomizing Parent Ids...."});
    if(Result<> randomizeResult = ClusterUtilities::RandomizeFeatureIds(featureParentIds, numParents); randomizeResult.invalid())
    {
      return mergeWarnings(std::move(randomizeResult));
    }
  }

  return result;
}
