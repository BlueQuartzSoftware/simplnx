#include "ComputeKernelAvgMisorientationsDirect.hpp"

#include "ComputeKernelAvgMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelData3DAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <algorithm>
#include <chrono>

using namespace nx::core;

namespace
{
/**
 * @class FindKernelAvgMisorientationsImpl
 * @brief Calculates direct KAM values for a three-dimensional range.
 *
 * ParallelData3DAlgorithm copies this body for its ranges. The caller disables
 * parallel scheduling when a selected array is out-of-core. This worker does
 * not provide a general DataStructure or DataStore thread-safety guarantee.
 */
class FindKernelAvgMisorientationsImpl
{
public:
  /**
   * @brief Initializes a direct KAM range worker.
   * @param progressMessenger Creates range-local progress messengers.
   * @param dataStructure Provides the selected arrays and Image Geometry.
   * @param inputValues Identifies the selected arrays and KAM settings.
   * @param shouldCancel Signals cancellation.
   * @pre Each argument remains valid while the parallel algorithm executes.
   */
  FindKernelAvgMisorientationsImpl(ProgressMessageHelper& progressMessenger, DataStructure& dataStructure, const ComputeKernelAvgMisorientationsInputValues* inputValues,
                                   const std::atomic_bool& shouldCancel)
  : m_ProgressMessageHelper(progressMessenger)
  , m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
    m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  }

  /**
   * @brief Calculates KAM values for one half-open three-dimensional range.
   * @param zStart Identifies the first Z plane.
   * @param zEnd Identifies the plane after the last Z plane.
   * @param yStart Identifies the first Y row.
   * @param yEnd Identifies the row after the last Y row.
   * @param xStart Identifies the first X column.
   * @param xEnd Identifies the column after the last X column.
   *
   * Valid focal cells average their admitted kernel neighbors in degrees.
   * Invalid focal cells receive zero.
   */
  void convert(size_t zStart, size_t zEnd, size_t yStart, size_t yEnd, size_t xStart, size_t xEnd) const
  {
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

    auto& kernelAvgMisorientationsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->KernelAverageMisorientationsArrayName);
    auto& kernelAvgMisorientations = kernelAvgMisorientationsArray.getDataStoreRef();

    const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
    SizeVec3 udims = imageGeom.getDimensions();

    ebsdlib::QuatD q1;
    ebsdlib::QuatD q2;

    usize counter = 0;
    usize increment = std::max(static_cast<usize>(1), (zEnd - zStart) / 100);

    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    const usize xPoints = udims[0];
    const usize yPoints = udims[1];
    const usize zPoints = udims[2];
    const usize kernelX = static_cast<usize>(kernelSize[0]);
    const usize kernelY = static_cast<usize>(kernelSize[1]);
    const usize kernelZ = static_cast<usize>(kernelSize[2]);
    for(size_t plane = zStart; plane < zEnd; plane++)
    {
      if(counter > increment)
      {
        progressMessenger.sendProgressMessage(counter);
        counter = 0;
      }

      for(size_t row = yStart; row < yEnd; row++)
      {
        if(m_ShouldCancel)
        {
          return;
        }

        const usize neighborYMin = row - std::min(row, kernelY);
        const usize neighborYMax = row + std::min(kernelY, (yPoints - 1) - row);
        for(size_t col = xStart; col < xEnd; col++)
        {
          size_t point = (plane * xPoints * yPoints) + (row * xPoints) + col;
          if(featureIds[point] > 0 && cellPhases[point] > 0)
          {
            float totalMisorientation = 0.0f;
            usize numVoxel = 0;

            size_t quatIndex = point * 4;
            q1[0] = quats[quatIndex];
            q1[1] = quats[quatIndex + 1];
            q1[2] = quats[quatIndex + 2];
            q1[3] = quats[quatIndex + 3];

            const usize neighborXMin = col - std::min(col, kernelX);
            const usize neighborXMax = col + std::min(kernelX, (xPoints - 1) - col);
            const usize neighborZMin = plane - std::min(plane, kernelZ);
            const usize neighborZMax = plane + std::min(kernelZ, (zPoints - 1) - plane);

            // The clamped bounds keep maximum valid kernel radii from
            // overflowing when they form the inclusive upper bounds.
            for(usize zIdx = neighborZMin;; zIdx++)
            {
              for(usize yIdx = neighborYMin;; yIdx++)
              {
                for(usize xIdx = neighborXMin;; xIdx++)
                {
                  const usize neighborIdx = (zIdx * xPoints * yPoints) + (yIdx * xPoints) + xIdx;
                  // Same-feature mode admits the focal feature. Same-phase mode
                  // admits positive feature IDs in the focal phase.
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

                  if(xIdx == neighborXMax)
                  {
                    break;
                  }
                }

                if(yIdx == neighborYMax)
                {
                  break;
                }
              }

              if(zIdx == neighborZMax)
              {
                break;
              }
            }
            // The focal cell contributes itself, so the denominator is nonzero.
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
    progressMessenger.sendProgressMessage(counter);
  }

  /**
   * @brief Calculates KAM values for a parallel range.
   * @param range Identifies the half-open X, Y, Z range.
   */
  void operator()(const Range3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  ProgressMessageHelper& m_ProgressMessageHelper;
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;
};

} // namespace

ComputeKernelAvgMisorientationsDirect::ComputeKernelAvgMisorientationsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                             const ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeKernelAvgMisorientationsDirect::~ComputeKernelAvgMisorientationsDirect() noexcept = default;

Result<> ComputeKernelAvgMisorientationsDirect::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = imageGeom.getDimensions();

  MessageHelper messageHelper(m_MessageHandler);
  ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();

  progressMessageHelper.setMaxProgresss(udims[2] * udims[1] * udims[0]);
  progressMessageHelper.setProgressMessageTemplate("Finding Kernel Average Misorientations || {:.2f}%");

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
