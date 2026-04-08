#include "ComputeKernelAvgMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

using namespace nx::core;

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

  const auto xPoints = static_cast<int64>(udims[0]);
  const auto yPoints = static_cast<int64>(udims[1]);
  const auto zPoints = static_cast<int64>(udims[2]);
  const usize sliceSize = static_cast<usize>(xPoints * yPoints);
  const auto kernelSize = m_InputValues->KernelSize;
  const int32 kZ = kernelSize[2];
  const int32 kY = kernelSize[1];
  const int32 kX = kernelSize[0];

  // Get DataStore references for bulk I/O
  const auto& cellPhasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  auto& outputStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->KernelAverageMisorientationsArrayName).getDataStoreRef();

  // Cache ensemble-level crystalStructures locally (tiny array, avoids per-element OOC overhead)
  const auto& crystalStructuresStore = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStore.getNumberOfTuples();
  std::vector<uint32> crystalStructuresLocal(numCrystalStructures);
  crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresLocal.data(), numCrystalStructures));

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Slab-based processing: for each Z-plane, read a slab of input data
  // spanning [plane - kZ, plane + kZ] in Z. This covers all neighbor
  // lookups for voxels in this plane.
  //
  // Slab buffers hold (slabZCount * Y * X) elements for 1-component arrays
  // and (slabZCount * Y * X * 4) for quaternions.

  for(int64 plane = 0; plane < zPoints; plane++)
  {
    if(m_ShouldCancel)
    {
      break;
    }
    // Compute slab Z range (clamped to volume bounds)
    const int64 slabZMin = std::max(static_cast<int64>(0), plane - kZ);
    const int64 slabZMax = std::min(zPoints - 1, plane + kZ);
    const usize slabZCount = static_cast<usize>(slabZMax - slabZMin + 1);
    const usize slabTuples = slabZCount * sliceSize;

    // Read slab data via copyIntoBuffer (OOC-safe bulk I/O)
    const usize slabStartTuple = static_cast<usize>(slabZMin) * sliceSize;

    std::vector<int32> slabFeatureIds(slabTuples);
    featureIdsStore.copyIntoBuffer(slabStartTuple, nonstd::span<int32>(slabFeatureIds.data(), slabTuples));

    std::vector<int32> slabCellPhases(slabTuples);
    cellPhasesStore.copyIntoBuffer(slabStartTuple, nonstd::span<int32>(slabCellPhases.data(), slabTuples));

    std::vector<float32> slabQuats(slabTuples * 4);
    quatsStore.copyIntoBuffer(slabStartTuple * 4, nonstd::span<float32>(slabQuats.data(), slabTuples * 4));

    // Output buffer for this plane
    std::vector<float32> planeOutput(sliceSize, 0.0f);

    // Offset of current plane within the slab
    const usize planeOffsetInSlab = static_cast<usize>(plane - slabZMin) * sliceSize;

    for(int64 row = 0; row < yPoints; row++)
    {
      for(int64 col = 0; col < xPoints; col++)
      {
        const usize pointInSlab = planeOffsetInSlab + static_cast<usize>(row * xPoints + col);
        const usize pointInPlane = static_cast<usize>(row * xPoints + col);

        const int32 featureId = slabFeatureIds[pointInSlab];
        const int32 cellPhase = slabCellPhases[pointInSlab];

        if(featureId <= 0 || cellPhase <= 0)
        {
          planeOutput[pointInPlane] = 0.0f;
          continue;
        }

        // Extract center quaternion
        ebsdlib::QuatD q1;
        const usize q1Idx = pointInSlab * 4;
        q1[0] = slabQuats[q1Idx];
        q1[1] = slabQuats[q1Idx + 1];
        q1[2] = slabQuats[q1Idx + 2];
        q1[3] = slabQuats[q1Idx + 3];

        const uint32 laueClass = crystalStructuresLocal[static_cast<usize>(cellPhase)];

        float32 totalMisorientation = 0.0f;
        int32 numVoxel = 0;

        for(int32 j = -kZ; j <= kZ; j++)
        {
          const int64 nz = plane + j;
          if(nz < 0 || nz >= zPoints)
          {
            continue;
          }
          const usize nzInSlab = static_cast<usize>(nz - slabZMin) * sliceSize;

          for(int32 k = -kY; k <= kY; k++)
          {
            const int64 ny = row + k;
            if(ny < 0 || ny >= yPoints)
            {
              continue;
            }

            for(int32 l = -kX; l <= kX; l++)
            {
              const int64 nx = col + l;
              if(nx < 0 || nx >= xPoints)
              {
                continue;
              }

              const usize neighborInSlab = nzInSlab + static_cast<usize>(ny * xPoints + nx);

              if(slabFeatureIds[neighborInSlab] == featureId)
              {
                const usize q2Idx = neighborInSlab * 4;
                ebsdlib::QuatD q2;
                q2[0] = slabQuats[q2Idx];
                q2[1] = slabQuats[q2Idx + 1];
                q2[2] = slabQuats[q2Idx + 2];
                q2[3] = slabQuats[q2Idx + 3];

                ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(q1, q2);
                totalMisorientation += (axisAngle[3] * nx::core::Constants::k_180OverPiF);
                numVoxel++;
              }
            }
          }
        }

        planeOutput[pointInPlane] = (numVoxel > 0) ? (totalMisorientation / static_cast<float32>(numVoxel)) : 0.0f;
      }
    }

    // Write this plane's output via bulk I/O
    const usize planeStartTuple = static_cast<usize>(plane) * sliceSize;
    outputStore.copyFromBuffer(planeStartTuple, nonstd::span<const float32>(planeOutput.data(), sliceSize));
  }

  return {};
}
