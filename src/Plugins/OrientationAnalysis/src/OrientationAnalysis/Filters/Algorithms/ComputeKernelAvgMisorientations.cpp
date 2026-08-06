#include "ComputeKernelAvgMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/Utilities/ParallelData3DAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <algorithm>
#include <chrono>

using namespace nx::core;

namespace
{
class FindKernelAvgMisorientationsImpl
{
public:
  FindKernelAvgMisorientationsImpl(ComputeKernelAvgMisorientations* filter, DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues* inputValues,
                                   const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
    m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  }

  // For each valid focal cell in the chunk: walk the (2rx+1)x(2ry+1)x(2rz+1) kernel, admit
  // neighbors per the Use Feature Ids mode, accumulate symmetry-reduced misorientation via
  // LaueOps, and store the average (degrees) at the focal cell. Invalid focal cells get 0.
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
    const bool useFeatureIds = m_InputValues->UseFeatureIds;

    // Output Arrays
    auto& kernelAvgMisorientationsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->KernelAverageMisorientationsArrayName);
    auto& kernelAvgMisorientations = kernelAvgMisorientationsArray.getDataStoreRef();

    const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
    SizeVec3 udims = imageGeom.getDimensions();

    ebsdlib::QuatD q1;
    ebsdlib::QuatD q2;

    // messenger values
    usize counter = 0;
    usize increment = std::max(static_cast<usize>(1), (zEnd - zStart) / 100);


    auto xPoints = static_cast<int64_t>(udims[0]);
    auto yPoints = static_cast<int64_t>(udims[1]);
    auto zPoints = static_cast<int64_t>(udims[2]);
    for(size_t plane = zStart; plane < zEnd; plane++)
    {
      if(counter > increment)
      {
        m_Filter->sendThreadSafeProgressMessage(counter);
        counter = 0;
      }

      for(size_t row = yStart; row < yEnd; row++)
      {
        if(m_ShouldCancel)
        {
          return;
        }

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

            for(int32_t j = -kernelSize[2]; j < kernelSize[2] + 1; j++)
            {
              const int64_t zIdx = static_cast<int64_t>(plane) + j;
              if(zIdx < 0 || zIdx > zPoints - 1)
              {
                continue;
              }
              for(int32_t k = -kernelSize[1]; k < kernelSize[1] + 1; k++)
              {
                const int64_t yIdx = static_cast<int64_t>(row) + k;
                if(yIdx < 0 || yIdx > yPoints - 1)
                {
                  continue;
                }
                for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
                {
                  const int64_t xIdx = static_cast<int64_t>(col) + l;
                  if(xIdx < 0 || xIdx > xPoints - 1)
                  {
                    continue;
                  }
                  // All three indices are clamped in-bounds, so the flattened neighbor index is
                  // always valid; no separate negative-index guard is needed.
                  const auto neighborIdx = static_cast<size_t>((zIdx * xPoints * yPoints) + (yIdx * xPoints) + xIdx);
                  // Per-grain mode: neighbor must belong to the same feature as the central cell.
                  // Per-voxel mode (use_feature_ids == false): neighbor must be a valid cell
                  // (featureId > 0) of the same phase as the central cell.
                  const bool neighborContributes = useFeatureIds ? (featureIds[point] == featureIds[neighborIdx]) : (featureIds[neighborIdx] > 0 && cellPhases[neighborIdx] == cellPhases[point]);
                  if(neighborContributes)
                  {
                    quatIndex = neighborIdx * 4;
                    q2[0] = quats[quatIndex];
                    q2[1] = quats[quatIndex + 1];
                    q2[2] = quats[quatIndex + 2];
                    q2[3] = quats[quatIndex + 3];
                    uint32_t laueClass = crystalStructures[cellPhases[point]];
                    ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
                    totalMisorientation = totalMisorientation + (axisAngle[3] * nx::core::Constants::k_180OverPiF);
                    numVoxel++;
                  }
                }
              }
            }
            // numVoxel is always >= 1 here: the focal cell passes both neighbor gates (j=k=l=0)
            // and contributes a self-misorientation of 0 degrees.
            kernelAvgMisorientations[point] = totalMisorientation / static_cast<float>(numVoxel);
          }
          else
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
  ComputeKernelAvgMisorientations* m_Filter = nullptr;
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;
};

} // namespace

// -----------------------------------------------------------------------------
ComputeKernelAvgMisorientations::ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKernelAvgMisorientations::~ComputeKernelAvgMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ComputeKernelAvgMisorientations::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(counter);
}

Result<> ComputeKernelAvgMisorientations::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = imageGeom.getDimensions();

  m_Throttle.reset(udims[2] * udims[1] * udims[0], "Finding Kernel Average Misorientations");

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
