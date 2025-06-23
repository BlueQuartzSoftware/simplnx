#include "ComputeKernelAvgMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelData3DAlgorithm.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <chrono>

using namespace nx::core;

namespace
{
class FindKernelAvgMisorientationsImpl
{
public:
  FindKernelAvgMisorientationsImpl(ProgressMessageHelper& progressMessenger, DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues* inputValues,
                                   const std::atomic_bool& shouldCancel)
  : m_ProgressMessageHelper(progressMessenger)
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

    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

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
        progressMessenger.sendProgressMessage(counter, [&](usize currentProgress, usize maxProgress) {
          int32 percentComplete = CalculatePercentCompleteAsInt(currentProgress, maxProgress);
          return fmt::format(fmt::runtime(progressMessenger.getProgressMessageData().m_MessageTemplate), percentComplete);
        });
        counter = 0;
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

              if(plane + j < 0 || plane + j > zPoints - 1)
              {
                continue;
              }
              const int64_t jStride = j * xPoints * yPoints;
              for(int32_t k = -kernelSize[1]; k < kernelSize[1] + 1; k++)
              {
                if(row + k < 0 || row + k > yPoints - 1)
                {
                  continue;
                }
                const int64_t kStride = k * xPoints;
                for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
                {
                  if(col + l < 0 || col + l > xPoints - 1)
                  {
                    continue;
                  }
                  const int64_t neighbor = static_cast<int64_t>(point) + jStride + kStride + l;
                  if(neighbor >= 0 && featureIds[point] == featureIds[static_cast<size_t>(neighbor)])
                  {
                    quatIndex = neighbor * 4;
                    q2[0] = quats[quatIndex];
                    q2[1] = quats[quatIndex + 1];
                    q2[2] = quats[quatIndex + 2];
                    q2[3] = quats[quatIndex + 3];
                    OrientationF axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
                    totalMisorientation = totalMisorientation + (axisAngle[3] * nx::core::Constants::k_180OverPiF);
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
    progressMessenger.sendProgressMessage(counter);
  }

  void operator()(const Range3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  ProgressMessageHelper& m_ProgressMessageHelper;
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
ComputeKernelAvgMisorientations::ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeKernelAvgMisorientations::~ComputeKernelAvgMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeKernelAvgMisorientations::operator()()
{
  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = gridGeom->getDimensions();

  MessageHelper messageHelper(m_MessageHandler);
  ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();

  progressMessageHelper.setMaxProgresss(udims[2] * udims[1] * udims[0]);
  progressMessageHelper.setProgressMessageTemplate("Finding Kernel Average Misorientations || {}%");

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->CrystalStructuresArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->KernelAverageMisorientationsArrayName));
  algArrays.push_back(m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath));

  ParallelData3DAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range3D(0, udims[0], 0, udims[1], 0, udims[2]));
  parallelAlgorithm.requireArraysInMemory(algArrays);
  parallelAlgorithm.execute(FindKernelAvgMisorientationsImpl(progressMessageHelper, m_DataStructure, m_InputValues, m_ShouldCancel));

  return {};
}
