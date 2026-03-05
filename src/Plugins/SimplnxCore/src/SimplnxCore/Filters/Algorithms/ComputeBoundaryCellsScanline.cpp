#include "ComputeBoundaryCellsScanline.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundaryCellsScanline::ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryCellsScanline::~ComputeBoundaryCellsScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Counts boundary faces per voxel using chunk-sequential iteration.
 * OOC path: iterates chunks in order via loadChunk/getChunkLowerBounds/getChunkUpperBounds,
 * then Z-Y-X within each chunk. Same logic as ComputeBoundaryCellsDirect.
 */
Result<> ComputeBoundaryCellsScanline::operator()()
{
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  int ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStore.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize zIdx = chunkLowerBounds[0]; zIdx <= chunkUpperBounds[0]; zIdx++)
    {
      const int64 kStride = static_cast<int64>(zIdx) * dims[0] * dims[1];
      for(usize yIdx = chunkLowerBounds[1]; yIdx <= chunkUpperBounds[1]; yIdx++)
      {
        const int64 jStride = static_cast<int64>(yIdx) * dims[0];
        for(usize xIdx = chunkLowerBounds[2]; xIdx <= chunkUpperBounds[2]; xIdx++)
        {
          const int64 voxelIndex = kStride + jStride + static_cast<int64>(xIdx);
          int8 onSurf = 0;
          const int32 feature = featureIdsStore[voxelIndex];
          if(feature >= 0)
          {
            if(m_InputValues->IncludeVolumeBoundary)
            {
              if(dims[0] > 2 && (static_cast<int64>(xIdx) == 0 || static_cast<int64>(xIdx) == dims[0] - 1))
              {
                onSurf++;
              }
              if(dims[1] > 2 && (static_cast<int64>(yIdx) == 0 || static_cast<int64>(yIdx) == dims[1] - 1))
              {
                onSurf++;
              }
              if(dims[2] > 2 && (static_cast<int64>(zIdx) == 0 || static_cast<int64>(zIdx) == dims[2] - 1))
              {
                onSurf++;
              }

              if(onSurf > 0 && feature == 0)
              {
                onSurf = 0;
              }
            }

            // Loop over the 6 face neighbors of the voxel
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(static_cast<int64>(xIdx), static_cast<int64>(yIdx), static_cast<int64>(zIdx), dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

              if(featureIdsStore[neighborPoint] != feature && featureIdsStore[neighborPoint] > ignoreFeatureZeroVal)
              {
                onSurf++;
              }
            }
          }
          boundaryCellsStore[voxelIndex] = onSurf;
        }
      }
    }
  }
  return {};
}
