#include "NeighborOrientationCorrelation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

using namespace nx::core;

namespace
{
// Copy a single tuple from one voxel to another, in place, for any cell array type that
// participates in the transfer. copyTuple() is declared separately on IDataArray and
// INeighborList (there is no shared IArray::copyTuple), and StringArray offers neither, so
// the copy is dispatched on the concrete array interface. Legacy DREAM3D 6.5.171 copied
// numeric, NeighborList, and String cell arrays alike; this reproduces that.
void CopyArrayTuple(IArray& array, usize from, usize to)
{
  if(auto* dataArrayPtr = dynamic_cast<IDataArray*>(&array); dataArrayPtr != nullptr)
  {
    dataArrayPtr->copyTuple(from, to);
  }
  else if(auto* neighborListPtr = dynamic_cast<INeighborList*>(&array); neighborListPtr != nullptr)
  {
    neighborListPtr->copyTuple(from, to);
  }
  else if(auto* stringArrayPtr = dynamic_cast<StringArray*>(&array); stringArrayPtr != nullptr)
  {
    (*stringArrayPtr)[to] = (*stringArrayPtr)[from];
  }
}

// Collect every cell array that participates in the tuple transfer: the numeric DataArrays
// plus NeighborList and String arrays that are siblings of the confidence-index array, minus
// any the user marked ignored. Mirrors GenerateDataArrayList() but over IArray rather than
// IDataArray so the NeighborList and String types are included (matching legacy 6.5.171).
std::vector<std::shared_ptr<IArray>> GenerateTransferArrayList(const DataStructure& dataStructure, const DataPath& referencePath, const std::vector<DataPath>& ignoredDataPaths)
{
  std::vector<std::shared_ptr<IArray>> arrays;
  const DataPath parentPath = referencePath.getParent();
  const auto& parentGroup = dataStructure.getDataRefAs<BaseGroup>(parentPath);
  const std::set<std::shared_ptr<IArray>> childArrays = parentGroup.findAllChildrenOfType<IArray>();
  for(const auto& childArray : childArrays)
  {
    DataPath childArrayPath;
    for(const auto& childDataPath : childArray->getDataPaths())
    {
      if(parentPath == childDataPath.getParent())
      {
        childArrayPath = childDataPath;
      }
    }
    if(std::find(ignoredDataPaths.cbegin(), ignoredDataPaths.cend(), childArrayPath) == ignoredDataPaths.cend())
    {
      arrays.push_back(childArray);
    }
  }
  return arrays;
}
} // namespace

class NeighborOrientationCorrelationTransferDataImpl
{
public:
  NeighborOrientationCorrelationTransferDataImpl() = delete;
  NeighborOrientationCorrelationTransferDataImpl(const NeighborOrientationCorrelationTransferDataImpl&) = default;

  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelation* filterAlg, size_t totalPoints, const std::vector<int64>& bestNeighbor, std::shared_ptr<IArray> arrayPtr,
                                                 const std::atomic_bool& shouldCancel)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_BestNeighbor(bestNeighbor)
  , m_ArrayPtr(arrayPtr)
  , m_ShouldCancel(shouldCancel)
  {
  }
  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelationTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(const NeighborOrientationCorrelationTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(NeighborOrientationCorrelationTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~NeighborOrientationCorrelationTransferDataImpl() = default;

  void operator()() const
  {
    std::string arrayName = m_ArrayPtr->getName();
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      m_FilterAlg->sendThreadSafeProgressMessage(fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)));
      int64 neighbor = m_BestNeighbor[i];
      if(neighbor != -1)
      {
        CopyArrayTuple(*m_ArrayPtr, static_cast<usize>(neighbor), i);
      }
    }
  }

private:
  NeighborOrientationCorrelation* m_FilterAlg = nullptr;
  size_t m_TotalPoints = 0;
  // Reference, not a copy: one task exists per transferred array and bestNeighbor is
  // 8 bytes per voxel. All tasks finish before the referenced vector leaves scope
  // (ParallelTaskAlgorithm waits in its destructor inside the level-loop iteration).
  const std::vector<int64>& m_BestNeighbor;
  std::shared_ptr<IArray> m_ArrayPtr;
  const std::atomic_bool& m_ShouldCancel;
};

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               NeighborOrientationCorrelationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::~NeighborOrientationCorrelation() noexcept = default;

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.trySendMessage(message);
}

// -----------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ConfidenceIndexArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  size_t totalPoints = confidenceIndex.getNumberOfTuples();

  float misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::array<int32, 6> neighborSimCount = {};
  std::vector<int64> bestNeighbor(totalPoints, -1);
  const int32 startLevel = 6;

  ThrottledMessageHandler throttledMessenger(m_MessageHandler);

  for(int32 currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    for(int64 voxelIndex = 0; voxelIndex < static_cast<int64>(totalPoints); voxelIndex++)
    {
      // The label varies per level, so the text is assembled only when a message is due.
      throttledMessenger.queueMessage("Level '{}' of '{}' || Processing Data {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->Level,
                                      CalculatePercentComplete(voxelIndex, totalPoints));

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
        const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndexJ : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndexJ])
          {
            continue;
          }
          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndexJ];

          // Compare every unordered pair (J, K) of valid face neighbors: a pair whose
          // cells share a phase (> 0) and lie within the misorientation tolerance is
          // "similar" and credits both neighbors' counts. The misorientation is freshly
          // initialized to max() per pair so a mixed-phase or phase-0 pair can never
          // inherit the previous pair's value (legacy 6.5.171 defect, deviation D1).
          for(size_t faceIndexK = faceIndexJ + 1; faceIndexK < VoxelNeighbors<Image3D>::k_FaceNeighborCount; faceIndexK++)
          {
            if(!isValidFaceNeighbor[faceIndexK])
            {
              continue;
            }
            const int64 neighborPoint2 = voxelIndex + neighborVoxelIndexOffsets[faceIndexK];

            const uint32 laueClass = crystalStructures[cellPhases[neighborPoint2]];
            const ebsdlib::QuatD quat1(quats[neighborPoint2 * 4], quats[neighborPoint2 * 4 + 1], quats[neighborPoint2 * 4 + 2], quats[neighborPoint2 * 4 + 3]);
            const ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
            ebsdlib::AxisAngleDType axisAngle(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
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

        // Loop over the 6 face neighbors of the voxel and keep the neighbor with the
        // highest similarity count. 'best' must persist across the whole loop; resetting
        // it per neighbor degrades the argmax to "last neighbor with any similar pair"
        // (legacy 6.5.171 defect, deviation D3). Ties resolve to the LAST neighbor in
        // scan order via '>=' so that fully-tied neighborhoods (the common interior
        // case) pick the same neighbor as 6.5.171; the count must be > 0 so a cell with
        // no similar pairs is never replaced.
        int32 best = 0;
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }
          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

          if(neighborSimCount[faceIndex] > 0 && neighborSimCount[faceIndex] >= best)
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

    // Transfer stage: copy the winning neighbor's tuple into each replaced cell,
    // parallelized with one task per array. Every sibling cell array of the confidence-index
    // array takes part — numeric DataArrays plus NeighborList and String arrays — matching
    // legacy 6.5.171. Each task owns a single array, so the parallel writes never touch the
    // same array concurrently.
    std::vector<std::shared_ptr<IArray>> voxelArrays = GenerateTransferArrayList(m_DataStructure, m_InputValues->ConfidenceIndexArrayPath, m_InputValues->IgnoredDataArrayPaths);
    ParallelTaskAlgorithm parallelTask;
    for(const auto& arrayPtr : voxelArrays)
    {
      parallelTask.execute(NeighborOrientationCorrelationTransferDataImpl(this, totalPoints, bestNeighbor, arrayPtr, m_ShouldCancel));
    }
  }

  return {};
}
