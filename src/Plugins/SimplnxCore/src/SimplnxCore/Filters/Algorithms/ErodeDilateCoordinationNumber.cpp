#include "ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ErodeDilateCoordinationNumberInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::~ErodeDilateCoordinationNumber() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateCoordinationNumber::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64, 3> dims = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
  const usize dimZ = static_cast<usize>(dims[2]);

  // Find max feature ID using Z-slice batched reads
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  usize numFeatures = 0;
  {
    std::vector<int32> sliceBuf(sliceSize);
    for(int64 z = 0; z < dims[2]; z++)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(sliceBuf.data(), sliceSize));
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
  bool keepGoing = true;
  int32 counter = 1;

  // FeatureIds rolling window
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) { featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(featureIdSlices[slot].data(), sliceSize)); };

  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // Per-slice neighbors (O(3*sliceSize) replaces O(totalPoints) neighbors array)
  // Slot 0=z-1, slot 1=z, slot 2=z+1
  std::array<std::vector<int64>, 3> sliceNeighbors;
  for(auto& sn : sliceNeighbors)
  {
    sn.resize(sliceSize, -1);
  }

  // Per-slice coordination numbers (O(3*sliceSize) replaces O(totalPoints))
  std::array<std::vector<int32>, 3> sliceCoordination;
  for(auto& sc : sliceCoordination)
  {
    sc.resize(sliceSize, 0);
  }

  // Helper to transfer a single Z-slice across all arrays, for qualifying voxels
  auto transferSlice = [&](usize z, const std::vector<int64>& marks, const std::vector<int32>& coord) {
    // Filter marks: only transfer voxels meeting coordination threshold
    std::vector<int64> filteredMarks(sliceSize, -1);
    for(usize i = 0; i < sliceSize; i++)
    {
      if(coord[i] >= m_InputValues->CoordinationNumber && coord[i] > 0)
      {
        filteredMarks[i] = marks[i];
        counter++;
      }
    }
    for(const auto& voxelArray : voxelArrays)
    {
      SliceBufferedTransferOneZ(*voxelArray, filteredMarks, sliceSize, z, dimZ);
    }
  };

  while(counter > 0 && keepGoing)
  {
    counter = 0;
    if(!m_InputValues->Loop)
    {
      keepGoing = false;
    }

    // Clear per-slice arrays
    for(auto& sn : sliceNeighbors)
    {
      std::fill(sn.begin(), sn.end(), -1);
    }
    for(auto& sc : sliceCoordination)
    {
      std::fill(sc.begin(), sc.end(), 0);
    }

    // Initialize rolling window
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readFeatureIdSlice(zIdx + 1, 2);
        }
      }

      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = dims[0] * dims[1] * zIdx + dims[0] * yIdx + xIdx;
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          int32 coordination = 0;
          int32 most = 0;

          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

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

            if((featureName > 0 && feature == 0) || (featureName == 0 && feature > 0))
            {
              coordination = coordination + 1;
              featureCount[feature]++;
              const int32 current = featureCount[feature];
              if(current > most)
              {
                most = current;
                sliceNeighbors[1][inSlice] = neighborPoint;
              }
            }
          }
          sliceCoordination[1][inSlice] = coordination;

          // Reset featureCount for neighbors
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
            if(feature > 0)
            {
              featureCount[feature] = 0;
            }
          }
        }
      }

      // Transfer z-1 (complete after processing z)
      if(zIdx > 0)
      {
        transferSlice(static_cast<usize>(zIdx - 1), sliceNeighbors[0], sliceCoordination[0]);
      }

      // Rotate per-slice arrays
      std::swap(sliceNeighbors[0], sliceNeighbors[1]);
      std::swap(sliceNeighbors[1], sliceNeighbors[2]);
      std::fill(sliceNeighbors[2].begin(), sliceNeighbors[2].end(), -1);

      std::swap(sliceCoordination[0], sliceCoordination[1]);
      std::swap(sliceCoordination[1], sliceCoordination[2]);
      std::fill(sliceCoordination[2].begin(), sliceCoordination[2].end(), 0);
    }

    // Transfer last slice
    if(dims[2] > 0)
    {
      transferSlice(static_cast<usize>(dims[2] - 1), sliceNeighbors[0], sliceCoordination[0]);
    }
  }

  return {};
}
