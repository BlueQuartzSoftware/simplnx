#include "BadDataNeighborOrientationCheck.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

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
  size_t totalPoints = quats.getNumberOfTuples();

  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54900, message);
  }

  int64_t dims[3] = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  int32_t good = 1;
  int64_t neighbor = 0;
  int64_t column = 0, row = 0, plane = 0;

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = static_cast<int64_t>(-dims[0] * dims[1]);
  neighpoints[1] = static_cast<int64_t>(-dims[0]);
  neighpoints[2] = static_cast<int64_t>(-1);
  neighpoints[3] = static_cast<int64_t>(1);
  neighpoints[4] = static_cast<int64_t>(dims[0]);
  neighpoints[5] = static_cast<int64_t>(dims[0] * dims[1]);

  float w = 10000.0f;

  uint32_t phase1 = 0;

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  std::vector<int32_t> neighborCount(totalPoints, 0);

  int64_t progressInt = 0;
  auto start = std::chrono::steady_clock::now();
  for(size_t i = 0; i < totalPoints; i++)
  {
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      progressInt = static_cast<int64_t>((static_cast<float>(i) / totalPoints) * 100.0f);
      std::string ss = fmt::format("Processing Data '{}'% completed", progressInt);
      m_MessageHandler({IFilter::Message::Type::Info, ss});
      start = std::chrono::steady_clock::now();
    }

    if(!maskCompare->isTrue(i))
    {
      column = i % dims[0];
      row = (i / dims[0]) % dims[1];
      plane = i / (dims[0] * dims[1]);
      for(int32_t j = 0; j < 6; j++)
      {
        good = 1;
        neighbor = i + neighpoints[j];
        if(j == 0 && plane == 0)
        {
          good = 0;
        }
        if(j == 5 && plane == (dims[2] - 1))
        {
          good = 0;
        }
        if(j == 1 && row == 0)
        {
          good = 0;
        }
        if(j == 4 && row == (dims[1] - 1))
        {
          good = 0;
        }
        if(j == 2 && column == 0)
        {
          good = 0;
        }
        if(j == 3 && column == (dims[0] - 1))
        {
          good = 0;
        }
        if(good == 1 && maskCompare->isTrue(neighbor))
        {
          phase1 = crystalStructures[cellPhases[i]];
          QuatF quat1(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
          QuatF quat2(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);

          if(cellPhases[i] == cellPhases[neighbor] && cellPhases[i] > 0)
          {
            OrientationD axisAngle = orientationOps[phase1]->calculateMisorientation(quat1, quat2);
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

  const int32_t startLevel = 6;
  int32_t currentLevel = startLevel;
  int32_t counter = 0;

  while(currentLevel > m_InputValues->NumberOfNeighbors)
  {
    counter = 1;
    int32_t loopNumber = 0;
    while(counter > 0)
    {
      counter = 0;
      progressInt = 0;
      start = std::chrono::steady_clock::now();
      for(size_t i = 0; i < totalPoints; i++)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          progressInt = static_cast<int64_t>((static_cast<float>(i) / totalPoints) * 100.0f);
          std::string ss =
              fmt::format("Level '{}' of '{}' || Processing Data ('{}') '{}'% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->NumberOfNeighbors, loopNumber, progressInt);
          m_MessageHandler({IFilter::Message::Type::Info, ss});
          start = std::chrono::steady_clock::now();
        }

        if(neighborCount[i] >= currentLevel && !maskCompare->isTrue(i))
        {
          maskCompare->setValue(i, true);
          counter++;
          column = i % dims[0];
          row = (i / dims[0]) % dims[1];
          plane = i / (dims[0] * dims[1]);
          for(int64_t j = 0; j < 6; j++)
          {
            good = 1;
            neighbor = i + neighpoints[j];
            if(j == 0 && plane == 0)
            {
              good = 0;
            }
            if(j == 5 && plane == (dims[2] - 1))
            {
              good = 0;
            }
            if(j == 1 && row == 0)
            {
              good = 0;
            }
            if(j == 4 && row == (dims[1] - 1))
            {
              good = 0;
            }
            if(j == 2 && column == 0)
            {
              good = 0;
            }
            if(j == 3 && column == (dims[0] - 1))
            {
              good = 0;
            }
            if(good == 1 && !maskCompare->isTrue(neighbor))
            {
              QuatF quat1(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
              QuatF quat2(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);

              if(cellPhases[i] == cellPhases[neighbor] && cellPhases[i] > 0)
              {
                OrientationD axisAngle = orientationOps[phase1]->calculateMisorientation(quat1, quat2);
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
