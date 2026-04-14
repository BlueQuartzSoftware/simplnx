#include "ComputeBoundaryCellsDirect.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeBoundaryCellsDirect -- In-Core Algorithm
//
// Counts, for each voxel, how many of its 6 face-connected neighbors belong to
// a different feature. The output is an Int8 array with values in [0, 6].
//
// Data access pattern: This variant reads FeatureIds and writes BoundaryCells
// via operator[], which is efficient when the underlying DataStore is a
// contiguous in-memory buffer (pointer dereference). It uses pre-computed
// neighbor index offsets from NeighborUtilities.hpp so that each neighbor
// lookup is a single addition + array index.
//
// This is NOT suitable for out-of-core data because the 6-neighbor access
// pattern is spatially scattered (especially the +/-Z neighbors, which are
// dimX*dimY elements apart), causing chunk thrashing on chunked stores.
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeBoundaryCellsDirect::ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       const ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryCellsDirect::~ComputeBoundaryCellsDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Counts boundary faces per voxel using direct in-memory array indexing.
 *
 * The algorithm proceeds as follows:
 * 1. Retrieve image geometry dimensions and compute flat-index offsets for the
 *    6 face neighbors (-Z, -Y, -X, +X, +Y, +Z).
 * 2. For each voxel in Z-Y-X order:
 *    a. Optionally count volume-boundary faces (if the voxel sits on the edge
 *       of the image geometry and IncludeVolumeBoundary is enabled).
 *    b. For each of the 6 face neighbors, check if the neighbor is inside the
 *       volume and belongs to a different feature. If so, increment the count.
 *    c. Store the count in the BoundaryCells output array.
 *
 * The IgnoreFeatureZero flag controls whether neighbors with FeatureId == 0
 * are counted as boundary faces. When true, only neighbors with FeatureId > 0
 * that differ from the current voxel's feature contribute to the count.
 *
 * @return Result<> indicating success (empty errors vector) or cancellation.
 */
Result<> ComputeBoundaryCellsDirect::operator()()
{
  // -- Step 1: Retrieve geometry dimensions and set up neighbor offset tables --
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Get direct references to the underlying data stores. Safe here because
  // the dispatcher only selects this variant when stores are in-memory.
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  // Pre-compute the flat-index offsets for the 6 face neighbors. For a voxel
  // at flat index i, the -Z neighbor is at i - (dimX*dimY), the -Y neighbor
  // is at i - dimX, etc. This avoids recomputing these offsets per voxel.
  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  int32 feature = 0;
  int8 onSurf = 0;
  int64 neighborPoint = 0;

  // When IgnoreFeatureZero is true, ignoreFeatureZeroVal == 0, so the condition
  // `neighborFeature > 0` filters out feature-0 neighbors. When false,
  // ignoreFeatureZeroVal == -1, so `neighborFeature > -1` allows feature 0
  // to count as a boundary neighbor.
  int ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  int64 kStride = 0;
  int64 jStride = 0;

  // -- Step 2: Main Z-Y-X iteration over all voxels --
  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    // Check for user cancellation once per Z-slice to avoid overhead
    if(m_ShouldCancel)
    {
      return {};
    }
    kStride = dims[0] * dims[1] * zIdx;
    for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
    {
      jStride = dims[0] * yIdx;
      for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
      {
        int64 voxelIndex = kStride + jStride + xIdx;
        onSurf = 0;
        feature = featureIdsStore[voxelIndex];
        if(feature >= 0)
        {
          // -- Step 2a: Volume boundary contribution --
          // If the voxel is on the edge of the image geometry and the user
          // enabled IncludeVolumeBoundary, count each edge face. The dim > 2
          // guard avoids counting boundaries for trivially thin dimensions.
          // Feature 0 voxels on the boundary are excluded (reset to 0).
          if(m_InputValues->IncludeVolumeBoundary)
          {
            if(dims[0] > 2 && (xIdx == 0 || xIdx == dims[0] - 1))
            {
              onSurf++;
            }
            if(dims[1] > 2 && (yIdx == 0 || yIdx == dims[1] - 1))
            {
              onSurf++;
            }
            if(dims[2] > 2 && (zIdx == 0 || zIdx == dims[2] - 1))
            {
              onSurf++;
            }

            if(onSurf > 0 && feature == 0)
            {
              onSurf = 0;
            }
          }

          // -- Step 2b: Check 6 face neighbors --
          // computeValidFaceNeighbors returns a bool[6] indicating which
          // neighbors are inside the volume (boundary voxels have fewer
          // valid neighbors). For each valid neighbor, compare its feature
          // ID to the current voxel's feature ID.
          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            // Count this face as a boundary if the neighbor belongs to a
            // different feature AND passes the feature-zero filter.
            if(featureIdsStore[neighborPoint] != feature && featureIdsStore[neighborPoint] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }
        }
        // -- Step 2c: Store the boundary count for this voxel --
        boundaryCellsStore[voxelIndex] = onSurf;
      }
    }
  }
  return {};
}
