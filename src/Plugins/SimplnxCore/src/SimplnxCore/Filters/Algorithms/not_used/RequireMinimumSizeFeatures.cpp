#include "RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;


namespace
{

class RequireMinimumSizeFeaturesTransferDataImpl
{
public:
  RequireMinimumSizeFeaturesTransferDataImpl() = delete;
  RequireMinimumSizeFeaturesTransferDataImpl(const RequireMinimumSizeFeaturesTransferDataImpl&) = default;

  RequireMinimumSizeFeaturesTransferDataImpl(RequireMinimumSizeFeatures* filterAlg, usize totalPoints, const Int32AbstractDataStore& featureIds,
                                     const std::vector<int64>& neighborVoxelIndex, const std::shared_ptr<IDataArray>& dataArrayPtr, MessageHelper& messageHelper)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_NeighborsVoxelIndex(neighborVoxelIndex)
  , m_DataArrayPtr(dataArrayPtr)
  , m_FeatureIds(featureIds)
  , m_MessageHelper(messageHelper)
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
    for(usize i = 0; i < m_TotalPoints; i++)
    {
      if(m_TotalPoints % 100 == 0)
      {
        throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)); });
      }

      int32 currentFeatureId = m_FeatureIds.getValue(i);
      int64 currentNeighborFeatureId = m_NeighborsVoxelIndex[i];
      if(currentNeighborFeatureId >= 0)
      {
        if(currentFeatureId < 0 && m_FeatureIds.getValue(currentNeighborFeatureId) >= 0)
        {
          m_DataArrayPtr->copyTuple(currentNeighborFeatureId, i);
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
};

}


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
    for(size_t i = 0; i < numFeatures; i++)
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
  std::vector<bool> activeObjects = removeSmallFeatures(featureIdsStoreRef, featureNumCellsStoreRef, featurePhases, m_InputValues->PhaseNumber, m_InputValues->ApplySinglePhase, m_InputValues->MinAllowedFeaturesSize, errorReturn);
  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  assignBadVoxels(imageGeom.getDimensions(), featureNumCellsStoreRef);

  DataPath cellFeatureGroupPath = m_InputValues->FeatureNumCellsPath.getParent();
  size_t currentFeatureCount = featureNumCellsStoreRef.getNumberOfTuples();

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


void RequireMinimumSizeFeatures::assignBadVoxels( SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef)
{
  MessageHelper messageHelper(m_MessageHandler);

  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  usize totalPoints = featureIds.getNumberOfTuples();

  std::array<int64_t, 3> dims = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  std::vector<int64_t> neighborsVoxelIndex(totalPoints * featureIds.getNumberOfComponents(), -1);

  int32 good = 1;
  int64 neighborVoxelIdx = 0;

  // These are the offsets that are applied to a voxel index to get to a specific neighbor voxel
  std::array<int64_t, 6> neighborVoxelIndexOffsets = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};

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
    for(int64 k = 0; k < dims[2]; k++)
    {
      kstride = dims[0] * dims[1] * k;
      for(int64 j = 0; j < dims[1]; j++)
      {
        jstride = dims[0] * j;
        for(int64 i = 0; i < dims[0]; i++)
        {
          count = kstride + jstride + i;
          int32 currentFeatureId = featureIds.getValue(count);
          if(currentFeatureId < 0)
          {
            counter++;
            uint8 maxVoteCount = 0;
            for(size_t l = 0; l < neighborVoxelIndexOffsets.size(); l++)
            {
              good = 1;
              neighborVoxelIdx = count + neighborVoxelIndexOffsets[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }

              if(good == 1)
              {
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
            }

            // Reset the VoteCounter back to Zero...
            std::fill(voteCounter.begin(), voteCounter.end(), 0);
          }
        }
      }
    }

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

      taskRunner.execute(RequireMinimumSizeFeaturesTransferDataImpl(this, totalPoints, featureIds, neighborsVoxelIndex, voxelArray, messageHelper));
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
    // Now update the feature Ids
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsPath);
    taskRunner.setParallelizationEnabled(false); // Do this to make the next call synchronous
    taskRunner.execute(RequireMinimumSizeFeaturesTransferDataImpl(this, totalPoints, featureIds, neighborsVoxelIndex, featureIDataArray, messageHelper));

  }
}

// -----------------------------------------------------------------------------
std::vector<bool> RequireMinimumSizeFeatures::removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef, const Int32AbstractDataStore* featurePhases,
                                       int32_t phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize, Error& errorReturn)
{
  size_t totalPoints = featureIdsStoreRef.getNumberOfTuples();

  bool good = false;
  int32 gnum;

  size_t totalFeatures = featureNumCellsStoreRef.getNumberOfTuples();

  std::vector<bool> activeObjects(totalFeatures, true);

  for(size_t i = 1; i < totalFeatures; i++)
  {
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
  for(size_t i = 0; i < totalPoints; i++)
  {
    gnum = featureIdsStoreRef.getValue(i);
    if(!activeObjects[gnum])
    {
      featureIdsStoreRef.setValue(i, -1);
    }
  }
  return activeObjects;
}
