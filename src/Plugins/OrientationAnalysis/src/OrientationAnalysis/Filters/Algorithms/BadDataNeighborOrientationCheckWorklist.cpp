#include "BadDataNeighborOrientationCheckWorklist.hpp"

#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <deque>

using namespace nx::core;

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckWorklist::BadDataNeighborOrientationCheckWorklist(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                 const BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckWorklist::~BadDataNeighborOrientationCheckWorklist() noexcept = default;

// -----------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheckWorklist::operator()()
{
  const float misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize totalPoints = quats.getNumberOfTuples();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    return MakeErrorResult(-54900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
  }

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const int64 xyStride = dims[0] * dims[1];

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  // ===== Phase 1: Count matching good neighbors for each bad voxel =====
  for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(voxelIndex, totalPoints)); });

    if(!maskCompare->isTrue(voxelIndex))
    {
      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }
        const int64 neighborPoint = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[faceIndex];

        if(maskCompare->isTrue(neighborPoint))
        {
          if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
          {
            ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
            quat2.positiveOrientation();
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            if(axisAngle[3] < misorientationTolerance)
            {
              neighborCount[voxelIndex]++;
            }
          }
        }
      }
    }
  }

  // ===== Phase 2: Iteratively flip bad voxels using worklist =====
  constexpr int32 startLevel = 6;
  const int32 totalLevels = startLevel - m_InputValues->NumberOfNeighbors + 1;

  for(int32 currentLevel = startLevel; currentLevel >= m_InputValues->NumberOfNeighbors; currentLevel--)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Level '{}' of '{}' || Building worklist", (startLevel - currentLevel) + 1, totalLevels); });

    std::deque<usize> worklist;
    for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      if(neighborCount[voxelIndex] >= currentLevel && !maskCompare->isTrue(voxelIndex))
      {
        worklist.push_back(voxelIndex);
      }
    }

    while(!worklist.empty())
    {
      const usize voxelIndex = worklist.front();
      worklist.pop_front();

      if(maskCompare->isTrue(voxelIndex) || neighborCount[voxelIndex] < currentLevel)
      {
        continue;
      }

      maskCompare->setValue(voxelIndex, true);

      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }

        const int64 neighborPoint = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[faceIndex];

        if(!maskCompare->isTrue(neighborPoint))
        {
          if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
          {
            ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
            quat2.positiveOrientation();
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            if(axisAngle[3] < misorientationTolerance)
            {
              neighborCount[neighborPoint]++;
              if(neighborCount[neighborPoint] >= currentLevel)
              {
                worklist.push_back(static_cast<usize>(neighborPoint));
              }
            }
          }
        }
      }
    }
  }

  return {};
}
