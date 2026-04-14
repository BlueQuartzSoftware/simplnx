#include "RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

#include <nonstd/span.hpp>

#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;
} // namespace

namespace
{

class RequireMinimumSizeFeaturesTransferDataImpl
{
public:
  RequireMinimumSizeFeaturesTransferDataImpl() = delete;
  RequireMinimumSizeFeaturesTransferDataImpl(const RequireMinimumSizeFeaturesTransferDataImpl&) = default;

  RequireMinimumSizeFeaturesTransferDataImpl(RequireMinimumSizeFeatures* filterAlg, usize totalPoints, const Int32AbstractDataStore& featureIds, const std::vector<int64>& neighborVoxelIndex,
                                             const std::shared_ptr<IDataArray>& dataArrayPtr, MessageHelper& messageHelper, const std::atomic_bool& shouldCancel)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_NeighborsVoxelIndex(neighborVoxelIndex)
  , m_DataArrayPtr(dataArrayPtr)
  , m_FeatureIds(featureIds)
  , m_MessageHelper(messageHelper)
  , m_ShouldCancel(shouldCancel)
  {
  }
  RequireMinimumSizeFeaturesTransferDataImpl(RequireMinimumSizeFeaturesTransferDataImpl&&) = default;                // Move Constructor is Not Implemented
  RequireMinimumSizeFeaturesTransferDataImpl& operator=(const RequireMinimumSizeFeaturesTransferDataImpl&) = delete; // Copy Assignment is Not Implemented
  RequireMinimumSizeFeaturesTransferDataImpl& operator=(RequireMinimumSizeFeaturesTransferDataImpl&&) = delete;      // Move Assignment is Not Implemented

  ~RequireMinimumSizeFeaturesTransferDataImpl() = default;

  void operator()() const
  {
    ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
    std::string arrayName = m_DataArrayPtr->getName();
    usize prog = std::max(m_TotalPoints / 100ULL, 1ULL);
    for(usize voxelIndex = 0; voxelIndex < m_TotalPoints; voxelIndex++)
    {
      if(voxelIndex % prog == 0)
      {
        throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(voxelIndex, m_TotalPoints)); });
      }
      if(m_ShouldCancel)
      {
        return;
      }

      int32 currentFeatureId = m_FeatureIds.getValue(voxelIndex);
      int64 currentNeighborFeatureId = m_NeighborsVoxelIndex[voxelIndex];
      if(currentNeighborFeatureId >= 0)
      {
        if(currentFeatureId < 0 && m_FeatureIds.getValue(currentNeighborFeatureId) >= 0)
        {
          m_DataArrayPtr->copyTuple(currentNeighborFeatureId, voxelIndex);
        }
      }
    }
  }

private:
  RequireMinimumSizeFeatures* m_FilterAlg = nullptr;
  usize m_TotalPoints = 0;
  std::vector<int64> m_NeighborsVoxelIndex;
  const std::shared_ptr<IDataArray> m_DataArrayPtr;
  const Int32AbstractDataStore& m_FeatureIds;
  MessageHelper& m_MessageHelper;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       RequireMinimumSizeFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::~RequireMinimumSizeFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeatures::operator()()
{

  // Input Cell Level Data
  auto& featureIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

  // Input Feature Level Data
  auto& featureNumCellsStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureNumCellsPath).getDataStoreRef();

  // Optionally allow applying to a single phase
  auto* featurePhases = m_InputValues->ApplySinglePhase ? m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesPath)->getDataStore() : nullptr;
  if(m_InputValues->ApplySinglePhase && featurePhases != nullptr)
  {
    usize numFeatures = featurePhases->getNumberOfTuples();
    bool unavailablePhase = true;
    for(usize i = 0; i < numFeatures; i++)
    {
      if(featurePhases->getValue(i) == m_InputValues->PhaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss = fmt::format("The phase number {} is not available in the supplied Feature phases array with path {}", m_InputValues->PhaseNumber, m_InputValues->FeaturePhasesPath.toString());
      return MakeErrorResult(-5555, ss);
    }
  }

  Error errorReturn = {0, ""};
  std::vector<bool> activeObjects =
      removeSmallFeatures(featureIdsStoreRef, featureNumCellsStoreRef, featurePhases, m_InputValues->PhaseNumber, m_InputValues->ApplySinglePhase, m_InputValues->MinAllowedFeaturesSize, errorReturn);

  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  assignBadVoxels(imageGeom.getDimensions(), featureNumCellsStoreRef);

  if(m_ShouldCancel)
  {
    return {};
  }
  DataPath cellFeatureGroupPath = m_InputValues->FeatureNumCellsPath.getParent();
  usize currentFeatureCount = featureNumCellsStoreRef.getNumberOfTuples();

  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    if(value)
    {
      count++;
    }
  }
  std::string message = fmt::format("Feature Count Changed: Previous: {} New: {}", currentFeatureCount, count);
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});

  nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIdsStoreRef, currentFeatureCount, m_MessageHandler, m_ShouldCancel);

  return {};
}

/**
 * @brief Iteratively fills voxels belonging to removed features (featureId < 0)
 * by majority-voting among their 6 face-neighbors.
 *
 * OOC optimization: The voting scan uses a rolling 3-slice buffer for FeatureIds.
 * For each Z-slice, the current slice and its Z-neighbors (prev, next) are in
 * memory, so all 6 face-neighbor reads come from local buffers rather than
 * per-element OOC DataStore access. The buffer slides forward one Z-slice per
 * iteration. Only changed voxels are tracked and have their data arrays updated,
 * rather than iterating over all voxels for each data array.
 */
void RequireMinimumSizeFeatures::assignBadVoxels(SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef)
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage(fmt::format("Assigning voxels...."));

  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  usize totalPoints = featureIds.getNumberOfTuples();

  std::array<int64, 3> dims = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  // Track which voxels need data copied from a neighbor
  std::vector<int64> neighborsVoxelIndex(totalPoints, -1);

  int64 neighborVoxelIdx = 0;

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();
  usize counter = 1;
  int64 kstride = 0;
  int64 jstride = 0;

  std::vector<uint8> voteCounter(featureNumCellsStoreRef.getNumberOfTuples(), 0);

  // Chunked scan: read featureIds in chunks for the voting scan
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  while(counter != 0)
  {
    counter = 0;

    // Rolling 3-slice buffer: holds the previous, current, and next Z-slices so
    // that all 6 face-neighbor reads come from local memory. The buffer is advanced
    // one Z-slice at a time by swapping pointers and reading the next slice.
    // This eliminates per-element OOC DataStore access during the voting scan.
    std::vector<int32> slabBuf(3 * sliceSize, 0);
    int32* prevSlice = slabBuf.data();
    int32* curSlice = slabBuf.data() + sliceSize;
    int32* nextSlice = slabBuf.data() + 2 * sliceSize;

    featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice, sliceSize));
    if(dims[2] > 1)
    {
      featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice, sliceSize));
    }

    // Collect indices of voxels that get assigned this iteration
    std::vector<usize> changedVoxels;

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      kstride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        jstride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 globalIdx = kstride + jstride + xIdx;
          const int64 localIdx = yIdx * dims[0] + xIdx;
          int32 currentFeatureId = curSlice[localIdx];
          if(currentFeatureId < 0)
          {
            counter++;
            uint8 maxVoteCount = 0;
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              neighborVoxelIdx = globalIdx + neighborVoxelIndexOffsets[faceIndex];

              // Read neighbor from slab buffer
              int32 neighborFeatureId = 0;
              const int64 neighborOffset = neighborVoxelIndexOffsets[faceIndex];
              if(neighborOffset == -dims[0] * dims[1])
              {
                neighborFeatureId = prevSlice[localIdx];
              }
              else if(neighborOffset == dims[0] * dims[1])
              {
                neighborFeatureId = nextSlice[localIdx];
              }
              else
              {
                neighborFeatureId = curSlice[localIdx + neighborOffset];
              }

              if(neighborFeatureId >= 0)
              {
                voteCounter[neighborFeatureId]++;
                uint8 currentVoteCount = voteCounter[neighborFeatureId];
                if(currentVoteCount > maxVoteCount)
                {
                  maxVoteCount = currentVoteCount;
                  neighborsVoxelIndex[globalIdx] = neighborVoxelIdx;
                }
              }
            }

            if(neighborsVoxelIndex[globalIdx] >= 0)
            {
              changedVoxels.push_back(static_cast<usize>(globalIdx));
            }

            std::fill(voteCounter.begin(), voteCounter.end(), 0);
          }
        }
      }

      // Slide the slab window
      std::swap(prevSlice, curSlice);
      std::swap(curSlice, nextSlice);
      if(zIdx + 2 < dims[2])
      {
        featureIds.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice, sliceSize));
      }
    }

    // Skip transfer entirely if no voxels were assigned
    if(changedVoxels.empty())
    {
      break;
    }

    messageHelper.sendMessage(fmt::format("Remaining voxels: {} - Updating {} changed voxels... ", counter, changedVoxels.size()));

    // Transfer phase: only copy tuples for voxels that actually changed
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsPath, {});

    for(const auto& voxelArray : voxelArrays)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      for(const usize voxelIndex : changedVoxels)
      {
        int64 neighborIdx = neighborsVoxelIndex[voxelIndex];
        if(neighborIdx >= 0 && featureIds.getValue(neighborIdx) >= 0)
        {
          voxelArray->copyTuple(neighborIdx, voxelIndex);
        }
      }
    }

    // Reset neighborsVoxelIndex for changed voxels
    for(const usize voxelIndex : changedVoxels)
    {
      neighborsVoxelIndex[voxelIndex] = -1;
    }
  }
}

// -----------------------------------------------------------------------------
std::vector<bool> RequireMinimumSizeFeatures::removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef,
                                                                  const Int32AbstractDataStore* featurePhases, int32 phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize,
                                                                  Error& errorReturn)
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage(fmt::format("Removing small features...."));

  usize totalPoints = featureIdsStoreRef.getNumberOfTuples();

  bool good = false;

  usize totalFeatures = featureNumCellsStoreRef.getNumberOfTuples();

  std::vector<bool> activeObjects(totalFeatures, true);

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(!applyToSinglePhase)
    {
      if(featureNumCellsStoreRef.getValue(i) >= minAllowedFeatureSize)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
    else
    {
      if(featureNumCellsStoreRef.getValue(i) >= minAllowedFeatureSize || featurePhases->getValue(i) != phaseNumber)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!good)
  {
    errorReturn = Error{-1, "The minimum size is larger than the largest Feature.  All Features would be removed"};
    return activeObjects;
  }

  // Mark removed features' voxels with featureId = -1 using chunked bulk I/O.
  // The original per-element setValue(-1) caused a chunk operation per voxel.
  // Reading/writing in 64K chunks reduces chunk operations by ~64000x.
  // Only modified chunks are written back (tracked via the `modified` flag).
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));

    bool modified = false;
    for(usize i = 0; i < count; i++)
    {
      if(!activeObjects[featureIdBuf[i]])
      {
        featureIdBuf[i] = -1;
        modified = true;
      }
    }
    if(modified)
    {
      featureIdsStoreRef.copyFromBuffer(offset, nonstd::span<const int32>(featureIdBuf.get(), count));
    }
  }
  return activeObjects;
}
