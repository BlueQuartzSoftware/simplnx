#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

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
  float misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  usize totalPoints = quats.getNumberOfTuples();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54900, message);
  }

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  int64 column = 0, row = 0, plane = 0;

  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = static_cast<int64>(-dims[0] * dims[1]);
  neighpoints[1] = static_cast<int64>(-dims[0]);
  neighpoints[2] = static_cast<int64>(-1);
  neighpoints[3] = static_cast<int64>(1);
  neighpoints[4] = static_cast<int64>(dims[0]);
  neighpoints[5] = static_cast<int64>(dims[0] * dims[1]);

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  // Loop over every point finding the number of neighbors that fall within the
  // user defined angle tolerance.
  for(usize voxelIdx = 0; voxelIdx < totalPoints; voxelIdx++)
  {
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(voxelIdx, totalPoints)); });
    // If the mask was set to false, then we check this voxel
    if(!maskCompare->isTrue(voxelIdx))
    {
      // We precalculate the positive voxel quaternion and laue class here to prevent reading and recalculating it for each face below
      ebsdlib::QuatD quat1(quats[voxelIdx * 4], quats[voxelIdx * 4 + 1], quats[voxelIdx * 4 + 2], quats[voxelIdx * 4 + 3]);
      quat1.positiveOrientation();
      uint32 laueClass1 = crystalStructures[cellPhases[voxelIdx]];

      column = voxelIdx % dims[0];
      row = (voxelIdx / dims[0]) % dims[1];
      plane = voxelIdx / (dims[0] * dims[1]);

      // Check the 6 Faces of the voxel
      for(int32 faceIdx = 0; faceIdx < 6; faceIdx++)
      {
        int64 neighborIdx = static_cast<int64>(voxelIdx) + neighpoints[faceIdx];
        // clang-format off
        if((faceIdx == 0 && plane == 0) ||
           (faceIdx == 1 && row == 0) ||
           (faceIdx == 2 && column == 0) ||
           (faceIdx == 3 && column == (dims[0] - 1)) ||
           (faceIdx == 4 && row == (dims[1] - 1)) ||
           (faceIdx == 5 && plane == (dims[2] - 1)))
        {
          continue;
        }
        // clang-format on

        // Now compare the mask of the neighbor. If the mask is TRUE, i.e., that voxel
        // did not fail the threshold filter that most likely produced the mask array,
        // then we can look at that voxel.
        if(maskCompare->isTrue(neighborIdx))
        {
          // Both Cell Phases MUST be the same and be a valid Phase
          if(cellPhases[voxelIdx] == cellPhases[neighborIdx] && cellPhases[voxelIdx] > 0)
          {
            ebsdlib::QuatD quat2(quats[neighborIdx * 4], quats[neighborIdx * 4 + 1], quats[neighborIdx * 4 + 2], quats[neighborIdx * 4 + 3]);
            quat2.positiveOrientation();
            // Compute the Axis_Angle misorientation between those 2 quaternions
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            // if the angle is less than our tolerance, then we increment the neighbor count
            // for this voxel
            if(axisAngle[3] < misorientationTolerance)
            {
              neighborCount[voxelIdx]++;
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
  while(currentLevel > m_InputValues->NumberOfNeighbors)
  {
    counter = 1;
    int32 loopNumber = 0;
    while(counter > 0)
    {
      counter = 0; // Set this while control variable to zero
      for(usize voxelIdx = 0; voxelIdx < totalPoints; voxelIdx++)
      {
        throttledMessenger.sendThrottledMessage([&]() {
          return fmt::format("Level '{}' of '{}' || Processing Data ('{}') {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->NumberOfNeighbors, loopNumber,
                             CalculatePercentComplete(voxelIdx, totalPoints));
        });

        // We not compare the number-of-neighbors of the current voxel and if it
        // is > the current level and the mask is FALSE, then we drop into this
        // conditional. The first thing that happens in the conditional is that
        // the current voxel's mask value is set to TRUE.
        if(neighborCount[voxelIdx] >= currentLevel && !maskCompare->isTrue(voxelIdx))
        {
          maskCompare->setValue(voxelIdx, true); // current voxel's mask value is set to TRUE.
          counter++;                             // Increment the `counter` to force the loop to iterate again

          // We precalculate the positive voxel quaternion and laue class here to prevent reading and recalculating it for each face below
          ebsdlib::QuatD quat1(quats[voxelIdx * 4], quats[voxelIdx * 4 + 1], quats[voxelIdx * 4 + 2], quats[voxelIdx * 4 + 3]);
          quat1.positiveOrientation();
          uint32 laueClass1 = crystalStructures[cellPhases[voxelIdx]];

          // This whole section below is to now look at the neighbor voxels of the
          // current voxel that just got flipped to true. This is needed because
          // if any of those neighbors mask was `false` then its neighbor count
          // is now not correct and will be off-by-one. So we run _almost_ the same
          // loop code as above but checking the specific neighbors of the current
          // voxel. This part should be termed the "Update Neighbor's Neighbor Count"
          column = voxelIdx % dims[0]; // Calculate the column, row, plane
          row = (voxelIdx / dims[0]) % dims[1];
          plane = voxelIdx / (dims[0] * dims[1]);

          for(int64 j = 0; j < 6; j++) // Loop over each of the 6 neighbor faces
          {
            int64 neighborIdx = static_cast<int64>(voxelIdx) + neighpoints[j];
            // clang-format off
            // Do NOT even look at any voxel along the outside boundary of the volume
            if((j == 0 && plane == 0) ||
               (j == 1 && row == 0) ||
               (j == 2 && column == 0) ||
               (j == 3 && column == (dims[0] - 1)) ||
               (j == 4 && row == (dims[1] - 1)) ||
               (j == 5 && plane == (dims[2] - 1)))
            {
              continue;
            }
            // clang-format on

            // If the neighbor voxel's mask is false then ....
            if(!maskCompare->isTrue(neighborIdx))
            {
              // Make sure both cell's phase are identical and valid
              if(cellPhases[voxelIdx] == cellPhases[neighborIdx] && cellPhases[voxelIdx] > 0)
              {
                ebsdlib::QuatD quat2(quats[neighborIdx * 4], quats[neighborIdx * 4 + 1], quats[neighborIdx * 4 + 2], quats[neighborIdx * 4 + 3]);
                quat2.positiveOrientation();
                // Quaternion Math is not commutative so do not reorder
                ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                if(axisAngle[3] < misorientationTolerance)
                {
                  neighborCount[neighborIdx]++;
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
