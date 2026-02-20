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
    std::string arrayName = m_DataArrayPtr->getName();
    usize totalPoints = m_TotalPoints;
    auto throttledMessenger = m_MessageHelper.createThrottledMessenger(
        [arrayName, totalPoints](usize current) { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(current, totalPoints)); });
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      throttledMessenger.sendMessage(i);
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

// -----------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::operator()()
{
  size_t progress = 0;
  size_t totalProgress = 0;

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ConfidenceIndexArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  size_t totalPoints = confidenceIndex.getNumberOfTuples();

  float misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  int32 best = 0;
  int64 neighborPoint2 = 0;

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<int32> neighborDiffCount(totalPoints, 0);
  std::vector<int32> neighborSimCount(6, 0);
  std::vector<int64> bestNeighbor(totalPoints, -1);
  const int32 startLevel = 6;

  MessageHelper messageHelper(m_MessageHandler);

  int32 totalLevels = startLevel - m_InputValues->Level;
  auto throttledMessenger = messageHelper.createThrottledMessenger([totalLevels, totalPoints](int32 levelNum, usize voxelIdx) {
    return fmt::format("Level '{}' of '{}' || Processing Data {:.2f}% completed", levelNum, totalLevels, CalculatePercentComplete(voxelIdx, totalPoints));
  });

  for(int32 currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      throttledMessenger.sendMessage((startLevel - currentLevel) + 1, static_cast<usize>(voxelIndex));

      if(m_ShouldCancel)
      {
        break;
      }

      if(confidenceIndex[voxelIndex] < m_InputValues->MinConfidence)
      {
        int64 xIdx = voxelIndex % dims[0];
        int64 yIdx = (voxelIndex / dims[0]) % dims[1];
        int64 zIdx = voxelIndex / (dims[0] * dims[1]);
        // Loop over the 6 face neighbors of the voxel
        std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndexJ : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndexJ])
          {
            continue;
          }
          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndexJ];

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

          isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(size_t faceIndexK = faceIndexJ + 1; faceIndexK < k_FaceNeighborCount; faceIndexK++)
          {
            if(!isValidFaceNeighbor[faceIndexK])
            {
              continue;
            }
            neighborPoint2 = voxelIndex + neighborVoxelIndexOffsets[faceIndexK];

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

        // Loop over the 6 face neighbors of the voxel
        isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }
          best = 0;

          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

          if(neighborSimCount[faceIndex] > best)
          {
            best = neighborSimCount[faceIndex];
            bestNeighbor[voxelIndex] = neighborPoint;
          }
          neighborSimCount[faceIndex] = 0;
        }
      }
    }

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

    currentLevel = currentLevel - 1;
  }

  return {};
}
