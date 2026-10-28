#include "ComputeFeatureCentroids.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>
#include <utility>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
// Each bulk read contains 65,536 Feature IDs. This keeps the staging buffer cache-sized.
constexpr usize k_ChunkTuples = 65536;
constexpr uint64 k_PreferredOccupancyBytes = 32ULL * 1024ULL * 1024ULL;
constexpr uint64 k_MinimumOccupancyBytes = sizeof(uint64);
constexpr usize k_NoCandidate = std::numeric_limits<usize>::max();

// One entry exists only for a feature/axis pair touching both opposing faces.
// The gap fields survive ascending occupancy windows; window fields locate this batch's bits.
// Occupancy selects a seam, while the final Kahan pass retains voxel multiplicities.
struct AxisCandidate
{
  usize FeatureId = 0;
  usize Axis = 0;
  usize First = 0;
  usize Previous = 0;
  usize LargestGap = 0;
  usize Cut = 0;
  usize WindowStart = 0;
  usize WindowLength = 0;
  usize WordOffset = 0;
  bool HasOccupied = false;
  bool TiedLargestGap = false;
  bool Adjust = false;
};

bool WouldMultiplyOverflow(usize lhs, usize rhs)
{
  return rhs != 0 && lhs > std::numeric_limits<usize>::max() / rhs;
}

// Division/remainder avoids overflow from rounding cellCount upward before dividing.
usize WordCount(usize cellCount)
{
  return (cellCount / 64) + static_cast<usize>(cellCount % 64 != 0);
}

// Coordinates arrive in ascending order, including across words and nonempty windows.
// The cut is the occupied coordinate immediately before the largest empty circular gap.
void ObserveOccupiedCoordinate(AxisCandidate& candidate, usize coordinate)
{
  if(!candidate.HasOccupied)
  {
    candidate.First = coordinate;
    candidate.Previous = coordinate;
    candidate.HasOccupied = true;
    return;
  }

  const usize gap = coordinate - candidate.Previous;
  if(gap > candidate.LargestGap)
  {
    candidate.LargestGap = gap;
    candidate.Cut = candidate.Previous;
    candidate.TiedLargestGap = false;
  }
  else if(gap == candidate.LargestGap)
  {
    candidate.TiedLargestGap = true;
  }
  candidate.Previous = coordinate;
}
} // namespace

ComputeFeatureCentroids::ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureCentroidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureCentroids::~ComputeFeatureCentroids() noexcept = default;

const std::atomic_bool& ComputeFeatureCentroids::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeFeatureCentroids::operator()()
{
  const auto* featureIdsPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();

  auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CentroidsArrayPath, *featureIdsPtr, false, m_MessageHandler, &m_ShouldCancel);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }
  Result<> result = std::move(validateNumFeatResult);
  if(m_ShouldCancel)
  {
    return result;
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalFeatures = centroids.getNumberOfTuples();
  const usize xPoints = imageGeom.getNumXCells();
  const usize yPoints = imageGeom.getNumYCells();
  const usize zPoints = imageGeom.getNumZCells();

  // The three-component uint64 count vector has the largest feature-sized byte requirement.
  const bool featureBytesOverflow = WouldMultiplyOverflow(totalFeatures, 3 * sizeof(uint64));
  if(featureBytesOverflow || WouldMultiplyOverflow(xPoints, yPoints) || WouldMultiplyOverflow(xPoints * yPoints, zPoints))
  {
    return MergeResults(
        std::move(result),
        MakeErrorResult(-5320, fmt::format("ComputeFeatureCentroids cannot index Image Geometry '{}' with dimensions ({}, {}, {}) and {} feature tuples: working-size arithmetic overflows.",
                                           m_InputValues->ImageGeometryPath.toString(), xPoints, yPoints, zPoints, totalFeatures)));
  }

  // Feature-sized vectors avoid DataStore calls in the voxel accumulation loop.
  const usize featureElems3 = totalFeatures * 3;
  const usize featureElems2 = totalFeatures * 2;
  std::vector<float64> kahanSum(featureElems3, 0.0);
  std::vector<float64> kahanComp(featureElems3, 0.0);
  std::vector<uint64> voxelCount(featureElems3, 0);
  std::vector<uint64> rangeX(featureElems2, 0);
  std::vector<uint64> rangeY(featureElems2, 0);
  std::vector<uint64> rangeZ(featureElems2, 0);

  for(usize f = 0; f < totalFeatures; f++)
  {
    rangeX[f * 2] = std::numeric_limits<uint64>::max();
    rangeY[f * 2] = std::numeric_limits<uint64>::max();
    rangeZ[f * 2] = std::numeric_limits<uint64>::max();
  }

  const FloatVec3 origin = imageGeom.getOrigin();
  const FloatVec3 spacing = imageGeom.getSpacing();
  const usize totalVoxels = xPoints * yPoints * zPoints;
  const usize xySize = xPoints * yPoints;

  // Each chunk resolves coordinates and feature accumulation from local values.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize chunkCount = std::min(k_ChunkTuples, totalVoxels - offset);
    result = MergeResults(std::move(result), featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount)));
    if(result.invalid())
    {
      return result;
    }

    for(usize idx = 0; idx < chunkCount; idx++)
    {
      const int32 featureId = featureIdBuf[idx];
      // Feature zero is valid. Only negative or unassigned IDs are skipped.
      if(featureId < 0)
      {
        continue;
      }

      const usize flatIdx = offset + idx;
      const uint64 k = flatIdx % xPoints;
      const uint64 j = (flatIdx / xPoints) % yPoints;
      const uint64 i = flatIdx / xySize;
      const usize fid = static_cast<usize>(featureId);

      rangeX[fid * 2] = std::min(k, rangeX[fid * 2]);
      rangeX[(fid * 2) + 1] = std::max(k, rangeX[(fid * 2) + 1]);
      rangeY[fid * 2] = std::min(j, rangeY[fid * 2]);
      rangeY[(fid * 2) + 1] = std::max(j, rangeY[(fid * 2) + 1]);
      rangeZ[fid * 2] = std::min(i, rangeZ[fid * 2]);
      rangeZ[(fid * 2) + 1] = std::max(i, rangeZ[(fid * 2) + 1]);

      const double vx = static_cast<double>(origin[0]) + (static_cast<double>(k) + 0.5) * static_cast<double>(spacing[0]);
      const double vy = static_cast<double>(origin[1]) + (static_cast<double>(j) + 0.5) * static_cast<double>(spacing[1]);
      const double vz = static_cast<double>(origin[2]) + (static_cast<double>(i) + 0.5) * static_cast<double>(spacing[2]);

      const std::array<double, 3> voxelCoords = {vx, vy, vz};
      for(usize c = 0; c < 3; c++)
      {
        const usize fi = fid * 3 + c;
        const double componentValue = voxelCoords[c] - kahanComp[fi];
        const double temp = kahanSum[fi] + componentValue;
        kahanComp[fi] = (temp - kahanSum[fi]) - componentValue;
        kahanSum[fi] = temp;
        voxelCount[fi]++;
      }
    }
  }

  // One bulk write publishes all finalized Kahan means.
  std::vector<float32> centroidsBuf(featureElems3, 0.0f);
  for(usize featureId = 0; featureId < totalFeatures; featureId++)
  {
    for(usize c = 0; c < 3; c++)
    {
      const usize fi = featureId * 3 + c;
      if(voxelCount[fi] > 0)
      {
        centroidsBuf[fi] = static_cast<float32>(kahanSum[fi] / static_cast<float64>(voxelCount[fi]));
      }
    }
  }
  if(m_InputValues->IsPeriodic)
  {
    m_MessageHandler.sendMessage({IFilter::Message::Type::Info, "Checking for periodic data."});
    // Reuse the range minima as a constant-time feature/axis-to-candidate lookup.
    std::array<std::vector<uint64>*, 3> rangeStores = {&rangeX, &rangeY, &rangeZ};
    const std::array<usize, 3> dims = {xPoints, yPoints, zPoints};
    std::vector<AxisCandidate> candidates;
    usize usefulWords = 0;
    constexpr usize preferredWords = static_cast<usize>(k_PreferredOccupancyBytes / sizeof(uint64));
    for(usize featureId = 0; featureId < totalFeatures; featureId++)
    {
      for(usize axis = 0; axis < 3; axis++)
      {
        const usize axisIdx = (featureId * 3) + axis;
        auto& rangeStore = *rangeStores[axis];
        const bool spansExtent = voxelCount[axisIdx] != 0 && dims[axis] > 1 && rangeStore[featureId * 2] == 0 && rangeStore[(featureId * 2) + 1] == dims[axis] - 1;
        if(spansExtent)
        {
          const usize candidateWords = WordCount(dims[axis]);
          usefulWords += std::min(candidateWords, preferredWords - usefulWords);
          rangeStore[featureId * 2] = candidates.size();
          AxisCandidate candidate;
          candidate.FeatureId = featureId;
          candidate.Axis = axis;
          candidates.push_back(candidate);
        }
        else
        {
          rangeStore[featureId * 2] = k_NoCandidate;
        }
      }
    }

    if(!candidates.empty())
    {
      {
        auto reservation = CacheMemoryBudgetManager::instance().reserveWorkingMemory(static_cast<uint64>(usefulWords) * sizeof(uint64));
        if(reservation.sizeBytes() < k_MinimumOccupancyBytes)
        {
          return MergeResults(std::move(result), MakeErrorResult(-5321, fmt::format("ComputeFeatureCentroids cannot scan periodic Feature IDs at '{}': shared working allowance granted {} bytes; at "
                                                                                    "least {} bytes are required. Retry after other working operations finish.",
                                                                                    m_InputValues->FeatureIdsArrayPath.toString(), reservation.sizeBytes(), k_MinimumOccupancyBytes)));
        }

        const usize availableWords = static_cast<usize>(reservation.sizeBytes() / sizeof(uint64));
        auto occupancy = std::make_unique<std::vector<uint64>>(availableWords);
        usize nextCandidate = 0;
        usize nextCoordinate = 0;
        usize occupancyScans = 0;
        while(nextCandidate < candidates.size())
        {
          if(m_ShouldCancel)
          {
            return result;
          }

          const usize firstCandidate = nextCandidate;
          usize lastCandidateExclusive = firstCandidate;
          usize usedWords = 0;
          // Full candidates share a batch. An oversized candidate resumes in ascending windows.
          while(nextCandidate < candidates.size() && usedWords < availableWords)
          {
            auto& candidate = candidates[nextCandidate];
            const usize remaining = dims[candidate.Axis] - nextCoordinate;
            const usize windowLength = std::min(remaining, (availableWords - usedWords) * 64);
            candidate.WindowStart = nextCoordinate;
            candidate.WindowLength = windowLength;
            candidate.WordOffset = usedWords;
            usedWords += WordCount(windowLength);
            lastCandidateExclusive = nextCandidate + 1;
            if(windowLength == remaining)
            {
              nextCandidate++;
              nextCoordinate = 0;
            }
            else
            {
              nextCoordinate += windowLength;
              break;
            }
          }

          std::fill_n(occupancy->data(), usedWords, uint64{0});
          for(usize offset = 0; offset < totalVoxels;)
          {
            if(m_ShouldCancel)
            {
              return result;
            }
            const usize chunkCount = std::min(k_ChunkTuples, totalVoxels - offset);
            result = MergeResults(std::move(result), featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount)));
            if(result.invalid())
            {
              return result;
            }
            for(usize idx = 0; idx < chunkCount; idx++)
            {
              const int32 featureId = featureIdBuf[idx];
              if(featureId < 0)
              {
                continue;
              }
              const usize flatIdx = offset + idx;
              const std::array<usize, 3> coordinates = {flatIdx % xPoints, (flatIdx / xPoints) % yPoints, flatIdx / xySize};
              const usize fid = static_cast<usize>(featureId);
              for(usize axis = 0; axis < 3; axis++)
              {
                const usize candidateIndex = static_cast<usize>((*rangeStores[axis])[fid * 2]);
                if(candidateIndex < firstCandidate || candidateIndex >= lastCandidateExclusive)
                {
                  continue;
                }
                const auto& candidate = candidates[candidateIndex];
                const usize coordinate = coordinates[axis];
                if(coordinate >= candidate.WindowStart && coordinate - candidate.WindowStart < candidate.WindowLength)
                {
                  const usize local = coordinate - candidate.WindowStart;
                  (*occupancy)[candidate.WordOffset + (local / 64)] |= uint64{1} << (local % 64);
                }
              }
            }
            offset += chunkCount;
          }

          for(usize candidateIndex = firstCandidate; candidateIndex < lastCandidateExclusive; candidateIndex++)
          {
            auto& candidate = candidates[candidateIndex];
            const usize wordCount = WordCount(candidate.WindowLength);
            for(usize wordIndex = 0; wordIndex < wordCount; wordIndex++)
            {
              if((wordIndex % 1024 == 0) && m_ShouldCancel)
              {
                return result;
              }
              uint64 word = (*occupancy)[candidate.WordOffset + wordIndex];
              while(word != 0)
              {
                const usize bit = static_cast<usize>(std::countr_zero(word));
                ObserveOccupiedCoordinate(candidate, candidate.WindowStart + (wordIndex * 64) + bit);
                word &= word - 1;
              }
            }
          }
          occupancyScans++;
          if(std::has_single_bit(occupancyScans))
          {
            m_MessageHandler.sendMessage({IFilter::Message::Type::Info, fmt::format("ComputeFeatureCentroids completed {} periodic occupancy scan(s).", occupancyScans)});
          }
        }
      }

      bool anyAdjusted = false;
      for(auto& candidate : candidates)
      {
        if(!candidate.HasOccupied)
        {
          continue;
        }
        const usize closingGap = dims[candidate.Axis] - candidate.Previous + candidate.First;
        if(closingGap > candidate.LargestGap)
        {
          candidate.LargestGap = closingGap;
          candidate.TiedLargestGap = false;
        }
        else if(closingGap == candidate.LargestGap)
        {
          candidate.TiedLargestGap = true;
        }
        candidate.Adjust = candidate.LargestGap > 1 && !candidate.TiedLargestGap;
        if(candidate.Adjust)
        {
          const usize fi = (candidate.FeatureId * 3) + candidate.Axis;
          kahanSum[fi] = 0.0;
          kahanComp[fi] = 0.0;
          anyAdjusted = true;
        }
      }

      if(anyAdjusted)
      {
        for(usize offset = 0; offset < totalVoxels;)
        {
          if(m_ShouldCancel)
          {
            return result;
          }
          const usize chunkCount = std::min(k_ChunkTuples, totalVoxels - offset);
          result = MergeResults(std::move(result), featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount)));
          if(result.invalid())
          {
            return result;
          }
          for(usize idx = 0; idx < chunkCount; idx++)
          {
            const int32 featureId = featureIdBuf[idx];
            if(featureId < 0)
            {
              continue;
            }
            const usize flatIdx = offset + idx;
            const std::array<usize, 3> coordinates = {flatIdx % xPoints, (flatIdx / xPoints) % yPoints, flatIdx / xySize};
            const usize fid = static_cast<usize>(featureId);
            for(usize axis = 0; axis < 3; axis++)
            {
              const usize candidateIndex = static_cast<usize>((*rangeStores[axis])[fid * 2]);
              if(candidateIndex == k_NoCandidate || !candidates[candidateIndex].Adjust)
              {
                continue;
              }
              const auto& candidate = candidates[candidateIndex];
              const double unwrapped = static_cast<double>(coordinates[axis]) + 0.5 + (coordinates[axis] <= candidate.Cut ? static_cast<double>(dims[axis]) : 0.0);
              const usize fi = (fid * 3) + axis;
              const double componentValue = unwrapped - kahanComp[fi];
              const double temp = kahanSum[fi] + componentValue;
              kahanComp[fi] = (temp - kahanSum[fi]) - componentValue;
              kahanSum[fi] = temp;
            }
          }
          offset += chunkCount;
        }

        for(const auto& candidate : candidates)
        {
          if(!candidate.Adjust)
          {
            continue;
          }
          const usize fi = (candidate.FeatureId * 3) + candidate.Axis;
          double wrappedIndexCenter = std::fmod(kahanSum[fi] / static_cast<double>(voxelCount[fi]), static_cast<double>(dims[candidate.Axis]));
          if(wrappedIndexCenter < 0.0)
          {
            wrappedIndexCenter += static_cast<double>(dims[candidate.Axis]);
          }
          centroidsBuf[fi] = static_cast<float32>(static_cast<double>(origin[candidate.Axis]) + (static_cast<double>(spacing[candidate.Axis]) * wrappedIndexCenter));
        }
        m_MessageHandler.sendMessage({IFilter::Message::Type::Info, "ComputeFeatureCentroids adjusted centroids of features that wrap the periodic boundary."});
      }
    }
  }

  if(m_ShouldCancel)
  {
    return result;
  }
  return MergeResults(std::move(result), centroids.copyFromBuffer(0, nonstd::span<const float32>(centroidsBuf.data(), featureElems3)));
}
