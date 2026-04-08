#include "ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ErodeDilateBadData::ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateBadData::~ErodeDilateBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateBadData::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64, 3> dims = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Find max feature ID using Z-slice batched reads
  usize numFeatures = 0;
  {
    std::vector<int32> sliceBuf(sliceSize);
    for(int64 z = 0; z < dims[2]; z++)
    {
      featureIds.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(sliceBuf.data(), sliceSize));
      for(usize i = 0; i < sliceSize; i++)
      {
        if(sliceBuf[i] > static_cast<int32>(numFeatures))
        {
          numFeatures = sliceBuf[i];
        }
      }
    }
  }

  std::vector<int32> featureCount(numFeatures + 1, 0);

  // FeatureIds rolling window for neighbor lookups
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) { featureIds.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(featureIdSlices[slot].data(), sliceSize)); };

  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // Per-slice mark arrays: marks[0]=z-1, marks[1]=z, marks[2]=z+1
  // Each entry is -1 (no transfer) or the global source index.
  // This replaces the O(totalPoints) neighbors array with O(3*sliceSize).
  std::array<std::vector<int64>, 3> marks;
  for(auto& m : marks)
  {
    m.resize(sliceSize);
  }

  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
  const usize dimZ = static_cast<usize>(dims[2]);

  // Helper to transfer a single Z-slice across all arrays
  auto transferSlice = [&](usize z, const std::vector<int64>& sliceMarks) {
    for(const auto& voxelArray : voxelArrays)
    {
      SliceBufferedTransferOneZ(*voxelArray, sliceMarks, sliceSize, z, dimZ);
    }
  };

  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    // Clear marks
    for(auto& m : marks)
    {
      std::fill(m.begin(), m.end(), -1);
    }

    // Initialize FeatureId rolling window
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance FeatureId rolling window
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readFeatureIdSlice(zIdx + 1, 2);
        }
      }

      // Find neighbors for bad voxels in this Z-slice
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          if(featureName == 0)
          {
            int32 most = 0;
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            if(!m_InputValues->XDirOn)
            {
              isValidFaceNeighbor[k_NegativeXNeighbor] = false;
              isValidFaceNeighbor[k_PositiveXNeighbor] = false;
            }
            if(!m_InputValues->YDirOn)
            {
              isValidFaceNeighbor[k_NegativeYNeighbor] = false;
              isValidFaceNeighbor[k_PositiveYNeighbor] = false;
            }
            if(!m_InputValues->ZDirOn)
            {
              isValidFaceNeighbor[k_NegativeZNeighbor] = false;
              isValidFaceNeighbor[k_PositiveZNeighbor] = false;
            }

            const int64 voxelIndex = xIdx + yIdx * dims[0] + zIdx * static_cast<int64>(sliceSize);
            const std::array<usize, 6> neighborInSlice = {
                inSlice,                                         // -Z
                static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                inSlice                                          // +Z
            };

            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];

              if(m_InputValues->Operation == detail::k_DilateIndex && feature > 0)
              {
                // Mark the good NEIGHBOR to be overwritten by this bad voxel.
                // The neighbor is in slot k_NeighborSlot[faceIndex] (0=z-1, 1=z, 2=z+1).
                marks[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] = voxelIndex;
              }
              if(feature > 0 && m_InputValues->Operation == detail::k_ErodeIndex)
              {
                featureCount[feature]++;
                const int32 current = featureCount[feature];
                if(current > most)
                {
                  most = current;
                  // Mark this bad voxel to be overwritten by the best neighbor
                  marks[1][inSlice] = neighborPoint;
                }
              }
            }
            if(m_InputValues->Operation == detail::k_ErodeIndex)
            {
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }
                const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
                featureCount[feature] = 0;
              }
            }
          }
        }
      }

      // Transfer z-1: all marks for z-1 are now complete (from bad voxels at z-2, z-1, z)
      if(zIdx > 0)
      {
        transferSlice(static_cast<usize>(zIdx - 1), marks[0]);
      }

      // Rotate marks: [0]=old[1], [1]=old[2], [2]=cleared
      std::swap(marks[0], marks[1]);
      std::swap(marks[1], marks[2]);
      std::fill(marks[2].begin(), marks[2].end(), -1);
    }

    // Transfer last slice
    if(dims[2] > 0)
    {
      transferSlice(static_cast<usize>(dims[2] - 1), marks[0]);
    }
  }

  return {};
}
