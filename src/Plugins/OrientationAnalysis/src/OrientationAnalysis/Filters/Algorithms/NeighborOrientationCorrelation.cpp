#include "NeighborOrientationCorrelation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#ifdef SIMPLNX_ENABLE_MULTICORE
#define RUN_TASK g->run
#else
#define RUN_TASK
#endif

#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

class NeighborOrientationCorrelationTransferDataImpl
{
public:
  NeighborOrientationCorrelationTransferDataImpl() = delete;
  NeighborOrientationCorrelationTransferDataImpl(const NeighborOrientationCorrelationTransferDataImpl&) = default;

  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelation* filterAlg, size_t totalPoints, const std::vector<int64_t>& bestNeighbor, std::shared_ptr<IDataArray> dataArrayPtr)
  : m_FilterAlg(filterAlg)
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
    auto start = std::chrono::steady_clock::now();
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      auto now = std::chrono::steady_clock::now();
      //// Only send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        float progress = static_cast<float>(i) / static_cast<float>(m_TotalPoints) * 100.0f;
        m_FilterAlg->updateProgress(fmt::format("Processing {}: {}% completed", m_DataArrayPtr->getName(), static_cast<int>(progress)));
        start = std::chrono::steady_clock::now();
      }
      int64_t neighbor = m_BestNeighbor[i];
      if(neighbor != -1)
      {
        m_DataArrayPtr->copyTuple(neighbor, i);
      }
    }
  }

private:
  NeighborOrientationCorrelation* m_FilterAlg = nullptr;
  size_t m_TotalPoints = 0;
  std::vector<int64_t> m_BestNeighbor;
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
const std::atomic_bool& NeighborOrientationCorrelation::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::operator()()
{
  size_t progress = 0;
  size_t totalProgress = 0;

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  const auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ConfidenceIndexArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  size_t totalPoints = confidenceIndex.getNumberOfTuples();

  float misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  int64_t dims[3] = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  size_t count = 1;
  int32_t best = 0;
  bool good = true;
  bool good2 = true;
  int64_t neighbor = 0;
  int64_t neighbor2 = 0;
  int64_t column = 0, row = 0, plane = 0;

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = static_cast<int64_t>(-dims[0] * dims[1]);
  neighpoints[1] = static_cast<int64_t>(-dims[0]);
  neighpoints[2] = static_cast<int64_t>(-1);
  neighpoints[3] = static_cast<int64_t>(1);
  neighpoints[4] = static_cast<int64_t>(dims[0]);
  neighpoints[5] = static_cast<int64_t>(dims[0] * dims[1]);

  uint32_t phase1 = 0;

  std::vector<int32_t> neighborDiffCount(totalPoints, 0);
  std::vector<int32_t> neighborSimCount(6, 0);
  std::vector<int64_t> bestNeighbor(totalPoints, -1);
  const int32_t startLevel = 6;
  float* currentQuatPtr = nullptr;

  for(int32_t currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    if(getCancel())
    {
      break;
    }

    int64_t progressInt = 0;
    auto start = std::chrono::steady_clock::now();
    for(size_t i = 0; i < totalPoints; i++)
    {
      auto now = std::chrono::steady_clock::now();
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        progressInt = static_cast<int64_t>((static_cast<float>(i) / totalPoints) * 100.0f);
        std::string ss = fmt::format("Level '{}' of '{}' || Processing Data '{}'% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->Level, progressInt);
        m_MessageHandler({IFilter::Message::Type::Info, ss});
        start = std::chrono::steady_clock::now();
      }

      if(confidenceIndex[i] < m_InputValues->MinConfidence)
      {
        count = 0;
        column = static_cast<int64_t>(i % dims[0]);
        row = (i / dims[0]) % dims[1];
        plane = i / (dims[0] * dims[1]);
        for(size_t j = 0; j < 6; j++)
        {
          good = true;
          neighbor = int64_t(i) + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = false;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            good = false;
          }
          if(j == 1 && row == 0)
          {
            good = false;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            good = false;
          }
          if(j == 2 && column == 0)
          {
            good = false;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            good = false;
          }
          if(good)
          {
            phase1 = crystalStructures[cellPhases[i]];
            QuatF quat1(quats[i * 4], quats[i * 4 + 1], quats[i * 4 + 2], quats[i * 4 + 3]);
            QuatF quat2(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);
            OrientationD axisAngle(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
            if(cellPhases[i] == cellPhases[neighbor] && cellPhases[i] > 0)
            {
              axisAngle = orientationOps[phase1]->calculateMisorientation(quat1, quat2);
            }
            if(axisAngle[3] > misorientationToleranceR)
            {
              neighborDiffCount[i]++;
            }
            for(size_t k = j + 1; k < 6; k++)
            {
              good2 = true;
              neighbor2 = int64_t(i) + neighpoints[k];
              if(k == 0 && plane == 0)
              {
                good2 = false;
              }
              if(k == 5 && plane == (dims[2] - 1))
              {
                good2 = false;
              }
              if(k == 1 && row == 0)
              {
                good2 = false;
              }
              if(k == 4 && row == (dims[1] - 1))
              {
                good2 = false;
              }
              if(k == 2 && column == 0)
              {
                good2 = false;
              }
              if(k == 3 && column == (dims[0] - 1))
              {
                good2 = false;
              }
              if(good2)
              {
                phase1 = crystalStructures[cellPhases[neighbor2]];
                quat1 = QuatF(quats[neighbor2 * 4], quats[neighbor2 * 4 + 1], quats[neighbor2 * 4 + 2], quats[neighbor2 * 4 + 3]);
                quat2 = QuatF(quats[neighbor * 4], quats[neighbor * 4 + 1], quats[neighbor * 4 + 2], quats[neighbor * 4 + 3]);
                axisAngle = OrientationD(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
                if(cellPhases[neighbor2] == cellPhases[neighbor] && cellPhases[neighbor2] > 0)
                {
                  axisAngle = orientationOps[phase1]->calculateMisorientation(quat1, quat2);
                }
                if(axisAngle[3] < misorientationToleranceR)
                {
                  neighborSimCount[j]++;
                  neighborSimCount[k]++;
                }
              }
            }
          }
        }
        for(size_t j = 0; j < 6; j++)
        {
          best = 0;
          good = true;
          neighbor = int64_t(i) + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = false;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            good = false;
          }
          if(j == 1 && row == 0)
          {
            good = false;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            good = false;
          }
          if(j == 2 && column == 0)
          {
            good = false;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            good = false;
          }
          if(good)
          {
            if(neighborSimCount[j] > best)
            {
              best = neighborSimCount[j];
              bestNeighbor[i] = neighbor;
            }
            neighborSimCount[j] = 0;
          }
        }
      }
    }

    if(getCancel())
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
      parallelTask.execute(NeighborOrientationCorrelationTransferDataImpl(this, totalPoints, bestNeighbor, dataArrayPtr));
    }

    currentLevel = currentLevel - 1;
  }

  return {};
}
