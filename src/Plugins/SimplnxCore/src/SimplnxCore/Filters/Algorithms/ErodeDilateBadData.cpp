#include "ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

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
const std::atomic_bool& ErodeDilateBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  const usize totalPoints = featureIds.getNumberOfTuples();

  std::vector<int64> neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  usize numFeatures = 0;
  for(usize i = 0; i < totalPoints; i++)
  {
    const int32 featureName = featureIds[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<int32> featureCount(numFeatures + 1, 0);

  // Z-slice buffering: maintain a rolling window of 3 adjacent Z-slices for
  // FeatureIds to avoid random OOC chunk access during neighbor lookups.
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling window: slot 0 = z-1, slot 1 = z (current), slot 2 = z+1
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    for(usize i = 0; i < sliceSize; i++)
    {
      featureIdSlices[slot][i] = featureIds[zOffset + i];
    }
  };

  // Helper to read a FeatureId from the rolling buffer.
  // neighborSlot: 0 = z-1, 1 = z (current), 2 = z+1
  // Face neighbor ordering: 0=-Z, 1=-Y, 2=-X, 3=+X, 4=+Y, 5=+Z
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    // Initialize rolling window: load z=0 into slot 1, z=1 into slot 2
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance rolling window for z > 0
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
          const int64 voxelIndex = xIdx + yIdx * dims[0] + zIdx * static_cast<int64>(sliceSize);
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          if(featureName == 0)
          {
            int32 most = 0;
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

            // Precompute neighbor in-slice indices for buffer lookups
            const std::array<usize, 6> neighborInSlice = {
                inSlice,                                           // -Z: same xy position in prev slice
                static_cast<usize>((yIdx - 1) * dims[0] + xIdx),  // -Y
                static_cast<usize>(yIdx * dims[0] + (xIdx - 1)),  // -X
                static_cast<usize>(yIdx * dims[0] + (xIdx + 1)),  // +X
                static_cast<usize>((yIdx + 1) * dims[0] + xIdx),  // +Y
                inSlice                                            // +Z: same xy position in next slice
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
                neighbors[neighborPoint] = voxelIndex;
              }
              if(feature > 0 && m_InputValues->Operation == detail::k_ErodeIndex)
              {
                featureCount[feature]++;
                const int32 current = featureCount[feature];
                if(current > most)
                {
                  most = current;
                  neighbors[voxelIndex] = neighborPoint;
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
    }

    // Sequential per-array transfer: process one array at a time so only one array's
    // chunks compete for the OOC cache. This avoids the chunk thrashing that occurs
    // when ParallelTaskAlgorithm processes multiple arrays simultaneously.
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

    for(const auto& voxelArray : voxelArrays)
    {
      if(voxelArray->getName() == m_InputValues->FeatureIdsArrayPath.getTargetName())
      {
        continue;
      }
      for(usize i = 0; i < totalPoints; i++)
      {
        const int64 neighbor = neighbors[i];
        if(neighbor >= 0)
        {
          const int32 featureName = featureIds[i];
          if((featureName == 0 && featureIds[neighbor] > 0 && m_InputValues->Operation == detail::k_ErodeIndex) ||
             (featureName > 0 && featureIds[neighbor] == 0 && m_InputValues->Operation == detail::k_DilateIndex))
          {
            voxelArray->copyTuple(neighbor, i);
          }
        }
      }
    }

    // Update FeatureIds last since the condition check depends on the original values
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
    for(usize i = 0; i < totalPoints; i++)
    {
      const int64 neighbor = neighbors[i];
      if(neighbor >= 0)
      {
        const int32 featureName = featureIds[i];
        if((featureName == 0 && featureIds[neighbor] > 0 && m_InputValues->Operation == detail::k_ErodeIndex) ||
           (featureName > 0 && featureIds[neighbor] == 0 && m_InputValues->Operation == detail::k_DilateIndex))
        {
          featureIDataArray->copyTuple(neighbor, i);
        }
      }
    }
  }

  return {};
}
