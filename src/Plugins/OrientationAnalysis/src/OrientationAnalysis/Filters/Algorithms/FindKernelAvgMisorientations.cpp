#include "FindKernelAvgMisorientations.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <chrono>

using namespace complex;

namespace
{
class FindKernelAvgMisorientationsImpl
{
public:
  FindKernelAvgMisorientationsImpl(FindKernelAvgMisorientations* filter, DataStructure& dataStructure, const FindKernelAvgMisorientationsInputValues* inputValues, const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(size_t zStart, size_t zEnd, size_t yStart, size_t yEnd, size_t xStart, size_t xEnd) const
  {
    // Input Arrays / Parameter Data
    const auto& cellPhasesArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
    const auto& cellPhases = cellPhasesArray.getDataStoreRef();
    const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
    const auto& featureIds = featureIdsArray.getDataStoreRef();
    const auto& quatsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
    const auto& quats = quatsArray.getDataStoreRef();
    const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
    const auto& crystalStructures = crystalStructuresArray.getDataStoreRef();
    const auto kernelSize = m_InputValues->KernelSize;

    // Output Arrays
    auto& kernelAvgMisorientationsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->KernelAverageMisorientationsArrayName);
    auto& kernelAvgMisorientations = kernelAvgMisorientationsArray.getDataStoreRef();

    std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

    auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);
    SizeVec3 udims = gridGeom->getDimensions();

    QuatD q1;
    QuatD q2;

    // messenger values
    usize counter = 0;
    usize increment = (zEnd - zStart) / 100;
    auto start = std::chrono::steady_clock::now();

    auto xPoints = static_cast<int64_t>(udims[0]);
    auto yPoints = static_cast<int64_t>(udims[1]);
    auto zPoints = static_cast<int64_t>(udims[2]);
    for(size_t plane = zStart; plane < zEnd; plane++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      if(counter > increment)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() < 1000)
        {
          m_Filter->sendThreadSafeProgressMessage(counter);
          counter = 0;
          start = std::chrono::steady_clock::now();
        }
      }

      for(size_t row = yStart; row < yEnd; row++)
      {
        for(size_t col = xStart; col < xEnd; col++)
        {
          size_t point = (plane * xPoints * yPoints) + (row * xPoints) + col;
          if(featureIds[point] > 0 && cellPhases[point] > 0)
          {
            float totalMisorientation = 0.0f;
            int32 numVoxel = 0;

            size_t quatIndex = point * 4;
            q1[0] = quats[quatIndex];
            q1[1] = quats[quatIndex + 1];
            q1[2] = quats[quatIndex + 2];
            q1[3] = quats[quatIndex + 3];

            uint32_t phase1 = crystalStructures[cellPhases[point]];
            for(int32_t j = -kernelSize[2]; j < kernelSize[2] + 1; j++)
            {
              size_t jStride = j * xPoints * yPoints;
              if(plane + j < 0 || plane + j > zPoints - 1)
              {
                continue;
              }
              for(int32_t k = -kernelSize[1]; k < kernelSize[1] + 1; k++)
              {
                size_t kStride = k * xPoints;
                if(row + k < 0 || row + k > yPoints - 1)
                {
                  continue;
                }
                for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
                {
                  if(col + l < 0 || col + l > xPoints - 1)
                  {
                    continue;
                  }
                  const size_t neighbor = point + (jStride) + (kStride) + (l);
                  if(featureIds[point] == featureIds[neighbor])
                  {
                    quatIndex = neighbor * 4;
                    q2[0] = quats[quatIndex];
                    q2[1] = quats[quatIndex + 1];
                    q2[2] = quats[quatIndex + 2];
                    q2[3] = quats[quatIndex + 3];
                    OrientationF axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
                    totalMisorientation = totalMisorientation + (axisAngle[3] * complex::Constants::k_180OverPiD);
                    numVoxel++;
                  }
                }
              }
            }
            kernelAvgMisorientations[point] = totalMisorientation / static_cast<float>(numVoxel);
            if(numVoxel == 0)
            {
              kernelAvgMisorientations[point] = 0.0f;
            }
          }
          if(featureIds[point] == 0 || cellPhases[point] == 0)
          {
            kernelAvgMisorientations[point] = 0.0f;
          }

          counter++;
        }
      }
    }
    m_Filter->sendThreadSafeProgressMessage(counter);
  }

  void operator()(const Range3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  FindKernelAvgMisorientations* m_Filter = nullptr;
  DataStructure& m_DataStructure;
  const FindKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
FindKernelAvgMisorientations::FindKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           FindKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindKernelAvgMisorientations::~FindKernelAvgMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindKernelAvgMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void FindKernelAvgMisorientations::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialPoint).count() < 1000)
  {
    return;
  }

  auto progressInt = static_cast<usize>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_TotalElements)) * 100.0f);
  std::string ss = fmt::format("Finding Kernel Average Misorientations || {}%", progressInt);
  m_MessageHandler(IFilter::Message::Type::Info, ss);

  m_LastProgressInt = progressInt;
  m_InitialPoint = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
Result<> FindKernelAvgMisorientations::operator()()
{
  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = gridGeom->getDimensions();

  // set up threadsafe messenger
  m_TotalElements = udims[2] * udims[1] * udims[0];

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->CrystalStructuresArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->KernelAverageMisorientationsArrayName));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath));

  ParallelData3DAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range3D(0, udims[0], 0, udims[1], 0, udims[2]));
  parallelAlgorithm.requireArraysInMemory(algArrays);
  parallelAlgorithm.execute(FindKernelAvgMisorientationsImpl(this, m_DataStructure, m_InputValues, m_ShouldCancel));

  return {};
}
