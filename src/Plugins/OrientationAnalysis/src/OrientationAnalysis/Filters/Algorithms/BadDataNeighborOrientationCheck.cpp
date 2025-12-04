#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <EbsdLib/Core/Orientation.hpp>
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

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
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

  int64 neighbor = 0;
  int64 column = 0, row = 0, plane = 0;

  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = static_cast<int64>(-dims[0] * dims[1]);
  neighpoints[1] = static_cast<int64>(-dims[0]);
  neighpoints[2] = static_cast<int64>(-1);
  neighpoints[3] = static_cast<int64>(1);
  neighpoints[4] = static_cast<int64>(dims[0]);
  neighpoints[5] = static_cast<int64>(dims[0] * dims[1]);

  float w = 10000.0f;

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  for(usize i = 0; i < totalPoints; i++)
  {
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(i, totalPoints)); });

    if(!maskCompare->isTrue(i))
    {
      column = i % dims[0];
      row = (i / dims[0]) % dims[1];
      plane = i / (dims[0] * dims[1]);
      for(int32 j = 0; j < 6; j++)
      {
        neighbor = i + neighpoints[j];
        // clang-format off
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
        else if(maskCompare->isTrue(neighbor))
        {
          ebsdlib::QuatD quat1(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
          ebsdlib::QuatD quat2(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);

          if(cellPhases[i] == cellPhases[neighbor] && cellPhases[i] > 0)
          {
            uint32 laueClass1 = crystalStructures[cellPhases[i]];
            // Quaternion Math is not commutative so do not reorder
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            w = axisAngle[3];
          }
          if(w < misorientationTolerance)
          {
            neighborCount[i]++;
          }
        }
      }
    }
  }

  const int32 startLevel = 6;
  int32 currentLevel = startLevel;
  int32 counter = 0;

  while(currentLevel > m_InputValues->NumberOfNeighbors)
  {
    counter = 1;
    int32 loopNumber = 0;
    while(counter > 0)
    {
      counter = 0;
      for(usize i = 0; i < totalPoints; i++)
      {
        throttledMessenger.sendThrottledMessage([&]() {
          return fmt::format("Level '{}' of '{}' || Processing Data ('{}') {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->NumberOfNeighbors, loopNumber,
                             CalculatePercentComplete(i, totalPoints));
        });

        if(neighborCount[i] >= currentLevel && !maskCompare->isTrue(i))
        {
          maskCompare->setValue(i, true);
          counter++;
          column = i % dims[0];
          row = (i / dims[0]) % dims[1];
          plane = i / (dims[0] * dims[1]);
          for(int64 j = 0; j < 6; j++)
          {
            neighbor = i + neighpoints[j];
            // clang-format off
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
            else if(!maskCompare->isTrue(neighbor))
            {
              ebsdlib::QuatD quat1(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
              ebsdlib::QuatD quat2(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);

              if(cellPhases[i] == cellPhases[neighbor] && cellPhases[i] > 0)
              {
                uint32 laueClass1 = crystalStructures[cellPhases[i]];
                // Quaternion Math is not commutative so do not reorder
                ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                w = axisAngle[3];
              }
              if(w < misorientationTolerance)
              {
                neighborCount[neighbor]++;
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
