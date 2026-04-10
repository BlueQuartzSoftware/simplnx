#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheck::BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheck::~BadDataNeighborOrientationCheck() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& BadDataNeighborOrientationCheck::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheck::operator()()
{
  const float misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
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
    // This really should NOT be happening as the path was verified during preflight, BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    return MakeErrorResult(-54900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
  }

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const std::vector<int64> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  const std::vector<FaceNeighborType> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  // Loop over every point finding the number of neighbors that fall within the
  // user defined angle tolerance.
  for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(voxelIndex, totalPoints)); });
    // If the mask was set to false, then we check this voxel
    if(!maskCompare->isTrue(voxelIndex))
    {
      // We precalculate the positive voxel quaternion and laue class here to prevent reading and recalculating it for each face below
      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

      int64 xIdx = voxelIndex % dims[0];
      int64 yIdx = (voxelIndex / dims[0]) % dims[1];
      int64 zIdx = voxelIndex / (dims[0] * dims[1]);

      // Loop over the 6 face neighbors of the voxel
      std::vector<bool> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }
        const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

        // Now compare the mask of the neighbor. If the mask is TRUE, i.e., that voxel
        // did not fail the threshold filter that most likely produced the mask array,
        // then we can look at that voxel.
        if(maskCompare->isTrue(neighborPoint))
        {
          // Both Cell Phases MUST be the same and be a valid Phase
          if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
          {
            ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
            quat2.positiveOrientation();
            // Compute the Axis_Angle misorientation between those 2 quaternions
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            // if the angle is less than our tolerance, then we increment the neighbor count
            // for this voxel
            if(axisAngle[3] < misorientationTolerance)
            {
              neighborCount[voxelIndex]++;
            }
          }
        }
      }
    }
  }

  constexpr int32 startLevel = 6;
  int32 currentLevel = startLevel;
  int32 counter = 0;

  // Now we loop over all the points again, but this time we do it as many times
  // as the user has requested to iteratively flip voxels
  while(currentLevel >= m_InputValues->NumberOfNeighbors)
  {
    counter = 1;
    int32 loopNumber = 0;
    while(counter > 0)
    {
      counter = 0; // Set this while control variable to zero
      for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        throttledMessenger.sendThrottledMessage([&] {
          return fmt::format("Level '{}' of '{}' || Processing Data ('{}') {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->NumberOfNeighbors, loopNumber,
                             CalculatePercentComplete(voxelIndex, totalPoints));
        });

        // We are comparing the number-of-neighbors of the current voxel, and if it
        // is > the current level and the mask is FALSE, then we drop into this
        // conditional. The first thing that happens in the conditional is that
        // the current voxel's mask value is set to TRUE.
        if(neighborCount[voxelIndex] >= currentLevel && !maskCompare->isTrue(voxelIndex))
        {
          maskCompare->setValue(voxelIndex, true); // the current voxel's mask value is set to TRUE.
          counter++;                               // Increment the `counter` to force the loop to iterate again

          // We precalculate the positive voxel quaternion and laue class here to prevent reading and recalculating it for each face below
          ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
          quat1.positiveOrientation();
          const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

          // This whole section below is to now look at the neighbor voxels of the
          // current voxel that just got flipped to true. This is needed because
          // if any of those neighbor's mask was `false`, then its neighbor count
          // is now not correct and will be off-by-one. So we run _almost_ the same
          // loop code as above but checking the specific neighbors of the current
          // voxel. This part should be termed the "Update Neighbor's Neighbor Count"
          int64 xIdx = voxelIndex % dims[0];
          int64 yIdx = (voxelIndex / dims[0]) % dims[1];
          int64 zIdx = voxelIndex / (dims[0] * dims[1]);

          // Loop over the 6 face neighbors of the voxel
          std::vector<bool> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            // If the neighbor voxel's mask is false, then ....
            if(!maskCompare->isTrue(neighborPoint))
            {
              // Make sure both cells phase values are identical and valid
              if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
              {
                ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
                quat2.positiveOrientation();
                // Quaternion Math is not commutative so do not reorder
                ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                if(axisAngle[3] < misorientationTolerance)
                {
                  neighborCount[neighborPoint]++;
                }
              }
            }
          }
        }
      }
      ++loopNumber;
    }
    currentLevel = currentLevel - 1;
  }

  return {};
}
