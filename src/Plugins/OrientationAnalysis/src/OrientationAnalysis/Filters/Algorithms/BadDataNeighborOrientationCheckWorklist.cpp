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
  // Double-precision pi keeps the strict comparison aligned with boundary-exact analytical fixtures.
  const double misorientationTolerance = static_cast<double>(m_InputValues->MisorientationTolerance) * numbers::pi_v<double> / 180.0;

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize totalPoints = quats.getNumberOfTuples();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // Direct callers can bypass preflight, so mask type errors return a Result.
    return MakeErrorResult(-54900,
                           fmt::format("Mask Array at '{}' could not be loaded; expected Bool or UInt8 backing. Underlying error: {}", m_InputValues->MaskArrayPath.toString(), exception.what()));
  }

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const int64 xyStride = dims[0] * dims[1];

  // The Image3D offsets retain six positions; runtime bounds checks omit Z neighbors for one-slice geometries.
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Validate indexes before orientationOps access. UnknownCrystalStructure remains a supported sentinel.
  const usize numOrientationOps = orientationOps.size();
  for(usize i = 0; i < crystalStructures.getSize(); ++i)
  {
    if(crystalStructures[i] >= numOrientationOps && crystalStructures[i] != ebsdlib::CrystalStructure::UnknownCrystalStructure)
    {
      return MakeErrorResult(
          -54901, fmt::format("Crystal structure at ensemble index {} has value {}, which is not a valid Laue-group index. Valid range is [0, {}).", i, crystalStructures[i], numOrientationOps));
    }
  }

  // The count array uses four bytes per voxel and retains cascade state separately from the mask.
  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  // Initialize matching-neighbor counts before worklist propagation.
  for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(voxelIndex, totalPoints)); });
    if(!maskCompare->isTrue(voxelIndex))
    {
      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClassIndex = crystalStructures[cellPhases[voxelIndex]];
      // UnknownCrystalStructure has no LaueOps entry and cannot participate in a match.
      if(laueClassIndex >= numOrientationOps)
      {
        continue;
      }

      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
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
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClassIndex]->calculateMisorientation(quat1, quat2);
            if(axisAngle[3] < misorientationTolerance)
            {
              neighborCount[voxelIndex]++;
            }
          }
        }
      }
    }
  }

  // Descend from six face neighbors to the requested count. One-slice geometries cannot meet the top levels.
  constexpr int32 startLevel = static_cast<int32>(k_NumFaceNeighbors);
  const int32 totalLevels = startLevel - m_InputValues->NumberOfNeighbors + 1;

  for(int32 currentLevel = startLevel; currentLevel >= m_InputValues->NumberOfNeighbors; currentLevel--)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

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
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize voxelIndex = worklist.front();
      worklist.pop_front();

      // Duplicate queue entries and prior flips cannot cause a second mask update.
      if(maskCompare->isTrue(voxelIndex) || neighborCount[voxelIndex] < currentLevel)
      {
        continue;
      }

      maskCompare->setValue(voxelIndex, true);

      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClassIndex = crystalStructures[cellPhases[voxelIndex]];
      // UnknownCrystalStructure has no LaueOps entry and cannot update neighbors.
      if(laueClassIndex >= numOrientationOps)
      {
        continue;
      }

      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      // A new good voxel can make an adjacent bad voxel eligible for the current level.
      const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
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
            // Keep quaternion order because misorientation calculation is directional.
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClassIndex]->calculateMisorientation(quat1, quat2);
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
