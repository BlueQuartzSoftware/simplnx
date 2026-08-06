#include "RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

using namespace nx::core;

namespace
{

class RequireMinimumSizeFeaturesTransferDataImpl
{
public:
  RequireMinimumSizeFeaturesTransferDataImpl() = delete;
  RequireMinimumSizeFeaturesTransferDataImpl(const RequireMinimumSizeFeaturesTransferDataImpl&) = default;

  RequireMinimumSizeFeaturesTransferDataImpl(RequireMinimumSizeFeatures* filterAlg, usize totalPoints, const Int32AbstractDataStore& featureIds, const std::vector<int64>& neighborVoxelIndex,
                                             const std::shared_ptr<IDataArray>& dataArrayPtr, const std::atomic_bool& shouldCancel)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_NeighborsVoxelIndex(neighborVoxelIndex)
  , m_DataArrayPtr(dataArrayPtr)
  , m_FeatureIds(featureIds)
  , m_ShouldCancel(shouldCancel)
  {
  }
  RequireMinimumSizeFeaturesTransferDataImpl(RequireMinimumSizeFeaturesTransferDataImpl&&) = default;                // Move Constructor is Not Implemented
  RequireMinimumSizeFeaturesTransferDataImpl& operator=(const RequireMinimumSizeFeaturesTransferDataImpl&) = delete; // Copy Assignment is Not Implemented
  RequireMinimumSizeFeaturesTransferDataImpl& operator=(RequireMinimumSizeFeaturesTransferDataImpl&&) = delete;      // Move Assignment is Not Implemented

  ~RequireMinimumSizeFeaturesTransferDataImpl() = default;

  void operator()() const
  {
    std::string arrayName = m_DataArrayPtr->getName();
    usize prog = std::max(m_TotalPoints / 100ULL, 1ULL);
    for(usize voxelIndex = 0; voxelIndex < m_TotalPoints; voxelIndex++)
    {
      if(voxelIndex % prog == 0)
      {
        m_FilterAlg->sendThreadSafeProgressMessage(fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(voxelIndex, m_TotalPoints)));
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
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::~RequireMinimumSizeFeatures() noexcept = default;

// -----------------------------------------------------------------------------
void RequireMinimumSizeFeatures::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.trySendMessage(message);
}

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
  m_MessageHandler.sendInfoMessage(message);

  nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIdsStoreRef, currentFeatureCount, m_MessageHandler, m_ShouldCancel);

  return {};
}

void RequireMinimumSizeFeatures::assignBadVoxels(SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef)
{
  m_MessageHandler.sendInfoMessage(fmt::format("Assigning voxels...."));

  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  usize totalPoints = featureIds.getNumberOfTuples();

  std::array<int64, 3> dims = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  std::vector<int64> neighborsVoxelIndex(totalPoints * featureIds.getNumberOfComponents(), -1);

  // int32 good = 1;
  int64 neighborVoxelIdx = 0;

  // These are the offsets that are applied to a voxel index to get to a specific neighbor voxel
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();
  usize counter = 1;
  int64 count = 0;
  int64 kstride = 0;
  int64 jstride = 0;

  // `voteCounter` serves as a vote counter array for determining which feature ID should
  // be assigned to "bad" voxels (those with featureId < 0 after small features
  // were removed). The array indexing is by feature ID. The largest value that could
  // be saved is 6 since there are only 6 face neighbors.
  std::vector<uint8> voteCounter(featureNumCellsStoreRef.getNumberOfTuples(), 0);

  while(counter != 0)
  {
    counter = 0;
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
          count = kstride + jstride + xIdx;
          int32 currentFeatureId = featureIds.getValue(count);
          if(currentFeatureId < 0)
          {
            counter++;
            uint8 maxVoteCount = 0;
            // Loop over the 6 face neighbors of the voxel
            const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              neighborVoxelIdx = count + neighborVoxelIndexOffsets[faceIndex];
              int32 neighborFeatureId = featureIds.getValue(neighborVoxelIdx);
              if(neighborFeatureId >= 0)
              {
                voteCounter[neighborFeatureId]++;
                uint8 currentVoteCount = voteCounter[neighborFeatureId];
                if(currentVoteCount > maxVoteCount)
                {
                  maxVoteCount = currentVoteCount;
                  neighborsVoxelIndex[count] = neighborVoxelIdx;
                }
              }
            }

            // Reset the VoteCounter back to Zero...
            std::fill(voteCounter.begin(), voteCounter.end(), 0);
          }
        }
      }
    }

    m_MessageHandler.sendInfoMessage(fmt::format("Remaining voxels: {} - Updating Data Arrays... ", counter));

    // Build up a list of the DataArrays that we are going to operate on.
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsPath, {});

    ParallelTaskAlgorithm taskRunner;
    taskRunner.setParallelizationEnabled(true);
    for(const auto& voxelArray : voxelArrays)
    {
      // We need to skip updating the FeatureIds until all the other arrays are updated
      // since we actually depend on the feature Ids values.
      if(voxelArray->getName() == m_InputValues->FeatureIdsPath.getTargetName())
      {
        continue;
      }

      taskRunner.execute(RequireMinimumSizeFeaturesTransferDataImpl(this, totalPoints, featureIds, neighborsVoxelIndex, voxelArray, m_ShouldCancel));
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
    // Now update the feature Ids
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsPath);
    taskRunner.setParallelizationEnabled(false); // Do this to make the next call synchronous
    taskRunner.execute(RequireMinimumSizeFeaturesTransferDataImpl(this, totalPoints, featureIds, neighborsVoxelIndex, featureIDataArray, m_ShouldCancel));
  }
}

// -----------------------------------------------------------------------------
std::vector<bool> RequireMinimumSizeFeatures::removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef,
                                                                  const Int32AbstractDataStore* featurePhases, int32 phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize,
                                                                  Error& errorReturn)
{
  m_MessageHandler.sendInfoMessage(fmt::format("Removing small features...."));

  usize totalPoints = featureIdsStoreRef.getNumberOfTuples();

  bool good = false;
  int32 gnum;

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
  for(usize i = 0; i < totalPoints; i++)
  {
    gnum = featureIdsStoreRef.getValue(i);
    if(!activeObjects[gnum])
    {
      featureIdsStoreRef.setValue(i, -1);
    }
  }
  return activeObjects;
}
