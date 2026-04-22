#include "NeighborOrientationCorrelation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#ifdef SIMPLNX_ENABLE_MULTICORE
#define RUN_TASK g->run
#else
#define RUN_TASK
#endif

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

class NeighborOrientationCorrelationTransferDataImpl
{
public:
  NeighborOrientationCorrelationTransferDataImpl() = delete;
  NeighborOrientationCorrelationTransferDataImpl(const NeighborOrientationCorrelationTransferDataImpl&) = default;

  NeighborOrientationCorrelationTransferDataImpl(MessageHelper& messageHelper, size_t totalPoints, const std::vector<int64>& bestNeighbor, std::shared_ptr<IDataArray> dataArrayPtr)
  : m_MessageHelper(messageHelper)
  , m_TotalPoints(totalPoints)
  , m_BestNeighbor(bestNeighbor)
  , m_DataArrayPtr(dataArrayPtr)
  {
  }
  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelationTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(const NeighborOrientationCorrelationTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(NeighborOrientationCorrelationTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~NeighborOrientationCorrelationTransferDataImpl() = default;

  void operator()() const
  {
    ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
    std::string arrayName = m_DataArrayPtr->getName();
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)); });
      int64 neighbor = m_BestNeighbor[i];
      if(neighbor != -1)
      {
        m_DataArrayPtr->copyTuple(neighbor, i);
      }
    }
  }

private:
  MessageHelper& m_MessageHelper;
  size_t m_TotalPoints = 0;
  std::vector<int64> m_BestNeighbor;
  std::shared_ptr<IDataArray> m_DataArrayPtr;
};

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               NeighborOrientationCorrelationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::~NeighborOrientationCorrelation() noexcept = default;

/**
 * @brief Checks the axis angles between the target voxel and its neighbors to calculate the neighborDiffCount and neighborSimCount values.
 * @param voxelIndex
 * @param isValidFaceNeighbor
 * @param cellPhases
 * @param quats
 * @param crystalStructures
 * @param neighborVoxelIndexOffsets
 * @param faceNeighborInternalIdx
 * @param misorientationToleranceR
 * @param orientation Ops
 * @param neighborDiffCount
 * @param neighborSimCount
 */
inline void checkAxisAngles(const int64& voxelIndex, const std::array<bool, 6>& isValidFaceNeighbor, const Int32AbstractDataStore& cellPhases, const Float32AbstractDataStore& quats,
                            const UInt32AbstractDataStore& crystalStructures, const std::array<int64, 6>& neighborVoxelIndexOffsets, const std::array<FaceNeighborType, 6>& faceNeighborInternalIdx,
                            const float misorientationToleranceR, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, std::vector<int32>& neighborDiffCount,
                            std::vector<int32>& neighborSimCount)
{
  int32 numCompatibleNeighbors = 0;

  // Loop over the 6 face neighbors of the voxel
  for(const auto& faceIndexJ : faceNeighborInternalIdx)
  {
    // If the face index is not valid, continue onto the next possible neighbor.
    if(!isValidFaceNeighbor[faceIndexJ])
    {
      continue;
    }

    // Calculate neighbor array index based on the voxel index and neighbor offsets vector
    const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndexJ];

    // Calculate the misorientation in order to determine if the angles are within the misorientation tolerance.
    // If the angles are compatible, increase neighborDiffCount for the voxel.
    uint32 laueClass = crystalStructures[cellPhases[voxelIndex]];
    ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
    ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
    ebsdlib::AxisAngleDType axisAngle(0.0, 0.0, 0.0, std::numeric_limits<double>::max());

    if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
    {
      axisAngle = orientationOps[laueClass]->calculateMisorientation(quat1, quat2);
    }
    if(axisAngle[3] > misorientationToleranceR)
    {
      neighborDiffCount[voxelIndex]++;
    }

    // Compare neighbor 1 against neighbor 2
    // Second neighbor is always after the first since all prior indices would have been checked already
    for(usize faceIndexK = faceIndexJ + 1; faceIndexK < k_FaceNeighborCount; faceIndexK++)
    {
      // If the face index is not valid, continue onto the next possible neighbor.
      if(!isValidFaceNeighbor[faceIndexK])
      {
        continue;
      }

      // Calculate neighbor array index based on the voxel index and neighbor offsets vector
      const int64 neighborPoint2 = voxelIndex + neighborVoxelIndexOffsets[faceIndexK];

      // Calculate the misorientation in order to determine if the angles are within the misorientation tolerance.
      // If the angles are compatible, increase neighborsSimCount for each face index.

      laueClass = crystalStructures[cellPhases[neighborPoint2]];
      quat1 = ebsdlib::QuatD(quats[neighborPoint2 * 4], quats[neighborPoint2 * 4 + 1], quats[neighborPoint2 * 4 + 2], quats[neighborPoint2 * 4 + 3]);
      quat2 = ebsdlib::QuatD(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
      axisAngle = ebsdlib::AxisAngleDType(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
      if(cellPhases[neighborPoint2] == cellPhases[neighborPoint] && cellPhases[neighborPoint2] > 0)
      {
        axisAngle = orientationOps[laueClass]->calculateMisorientation(quat1, quat2);
      }
      if(axisAngle[3] < misorientationToleranceR)
      {
        neighborSimCount[faceIndexJ]++;
        neighborSimCount[faceIndexK]++;
      }
    }
  }
}

/**
 * @brief Determines the bestNeighbor data for the specified voxel.
 * The bestNeighbor value is set based on the largest neighborSimCount value.
 * @param voxelIndex Array index for the target voxel.
 * @param isValidFaceNeighbor Array to check face neighbor validity.
 * @params neighborSimCount
 * @param neighborVoxelIndexOffsets
 * @param faceNeighborInternalIdx
 * @param best Best value for neighborSimCount - possible bug in its usage, and perhaps this does not need to be a parameter.
 * @param bestNeighbor Collection of best neighbor value indices indexed on the current voxel index.
 */
inline void findBestNeighbors(const int64& voxelIndex, const std::array<bool, 6>& isValidFaceNeighbor, std::vector<int32>& neighborSimCount, const std::array<int64, 6>& neighborVoxelIndexOffsets,
                              const std::array<FaceNeighborType, 6>& faceNeighborInternalIdx, std::vector<int64>& bestNeighbor)
{
  // Reset bestSimCount value.
  // This was originally created before the level was processed, but it is never used outside this method.
  // This was inside of the for loop in previous versions, causing it to be treated as a constant to check against.
  int32 bestSimCount = 0;

  // Loop over the 6 face neighbors of the voxel
  for(const auto& faceIndex : faceNeighborInternalIdx)
  {
    // If the face index is not valid, continue onto the next possible neighbor.
    if(!isValidFaceNeighbor[faceIndex])
    {
      continue;
    }

    // Calculate the neighbor point index
    const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

    // If the number of neighbor sims is better than the best, update the best and bestNeighbor values.
    // Best is always 0 in this function. Is this a bug? Why is the best value set at all?
    if(neighborSimCount[faceIndex] > bestSimCount)
    {
      bestSimCount = neighborSimCount[faceIndex];
      bestNeighbor[voxelIndex] = neighborPoint;
    }

    // Reset neighbor sim count
    neighborSimCount[faceIndex] = 0;
  }
}

/**
 * @brief Handles the processing for a single voxel of the ImageGeom.
 * If the confidence index for the voxel index is above or equal to the minimum confidence, this method does nothing.
 * @param totalPoints Number of tuples in the ImageGeom
 * @param voxelIndex Array index of the tuple currently being processed
 * @param cellPhases AbstractDataStore containing the ImageGeom's cell phases
 * @param quats AbstractDataStore containing the ImageGeom's quaturnions
 * @param crystalStructure AbstractDataStore containing the ImageGeom's crystal structure
 * @param dims The image geometry dimensions for determining neighbor indices.
 * @param misorientationToleranceR Misorientation tolerance for determining if two voxels are adequatly aligned.
 * @param orientationOps EBSD orientation laue ops
 * @param neighborVoxelIndexOffsets
 * @param faceNeighborInternalIdx
 * @param bestNeigbhor Vector of the best neighbor for each voxel index
 * @param minCompatibleNeigbhors Minimum aligned neighbors required for processing the voxel.
 */
inline void processVoxel(usize totalPoints, const int64& voxelIndex, const Int32AbstractDataStore& cellPhases, const Float32AbstractDataStore& quats, const UInt32AbstractDataStore& crystalStructures,
                         const std::array<int64, 3>& dims, const float misorientationToleranceR, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps,
                         const std::array<int64, 6>& neighborVoxelIndexOffsets, const std::array<FaceNeighborType, 6>& faceNeighborInternalIdx, std::vector<int64>& bestNeighbor,
                         int32 minCompatibleNeighbors)
{
  std::vector<int32> neighborDiffCount(totalPoints, 0);
  std::vector<int32> neighborSimCount(6, 0);

  // These three variables are only necessary to create valid face neighbor array.
  int64 xIdx = voxelIndex % dims[0];
  int64 yIdx = (voxelIndex / dims[0]) % dims[1];
  int64 zIdx = voxelIndex / (dims[0] * dims[1]);
  // Create an array to check the validity of 6 face neighbors in each of the subtasks.
  const std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

  checkAxisAngles(voxelIndex, isValidFaceNeighbor, cellPhases, quats, crystalStructures, neighborVoxelIndexOffsets, faceNeighborInternalIdx, misorientationToleranceR, orientationOps,
                  neighborDiffCount, neighborSimCount);

  // Check number of neighbors with compatible axis angles against the minimum number of compatible neighbors.
  if(neighborDiffCount[voxelIndex] >= minCompatibleNeighbors)
  {
    findBestNeighbors(voxelIndex, isValidFaceNeighbor, neighborSimCount, neighborVoxelIndexOffsets, faceNeighborInternalIdx, bestNeighbor);
  }
}

// -----------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::operator()()
{
  size_t progress = 0;
  size_t totalProgress = 0;

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ConfidenceIndexArrayPath).getDataStoreRef();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  size_t totalPoints = confidenceIndex.getNumberOfTuples();

  float misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // int64 neighborPoint2 = 0;

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Sets the bestNeighbor collection. std::vector is not safe for large data.
  std::vector<int64> bestNeighbor(totalPoints, -1);
  const int32 startLevel = 6;

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  // Iterate over levels 6 through (input) Level decreasing currentLevel by 1 after each iteration.
  // Former versions decremented currentLevel by 2 after each iteration.
  // This value is used to determine the minimum number of neighbors when processing a voxel.
  for(int32 currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    // Iterate over each voxel
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      throttledMessenger.sendThrottledMessage([&]() {
        return fmt::format("Level '{}' of '{}' || Processing Data {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->Level,
                           CalculatePercentComplete(voxelIndex, totalPoints));
      });

      // Allow cancelling before each voxel is processed.
      if(m_ShouldCancel)
      {
        break;
      }

      // Only process the voxel if the confidence index is less than the minimum confidence
      if(confidenceIndex[voxelIndex] >= m_InputValues->MinConfidence)
      {
        continue;
      }

      processVoxel(totalPoints, voxelIndex, cellPhases, quats, crystalStructures, dims, misorientationToleranceR, orientationOps, neighborVoxelIndexOffsets, faceNeighborInternalIdx, bestNeighbor, currentLevel);
    }
    // Finished iterating over voxels

    // Allow cancelling after each level is processed.
    if(m_ShouldCancel)
    {
      return {};
    }

    // Build up a list of the DataArrays that we are going to operate on.
    std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->ConfidenceIndexArrayPath, m_InputValues->IgnoredDataArrayPaths);
    // The idea for this parallel section is to parallelize over each Data Array that
    // will need it's data adjusted. This should go faster than before by about 2x.
    // Better speed up could be achieved if we had better data locality.
    ParallelTaskAlgorithm parallelTask;
    for(const auto& dataArrayPtr : voxelArrays)
    {
      parallelTask.execute(NeighborOrientationCorrelationTransferDataImpl(messageHelper, totalPoints, bestNeighbor, dataArrayPtr));
    }
  }

  return {};
}
