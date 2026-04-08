#include "ComputeFeatureNeighborsScanline.hpp"

#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <nonstd/span.hpp>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeFeatureNeighborsScanline::ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 const ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighborsScanline::~ComputeFeatureNeighborsScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Computes feature neighbor lists using Z-slice bulk I/O with per-face
 * surface area accumulation.
 *
 * OOC path: Reads FeatureIds one Z-slice at a time via copyIntoBuffer using a
 * 3-slice rolling window (prev/cur/next) to resolve all 6 face neighbors.
 * BoundaryCells output is written per-slice via copyFromBuffer.
 *
 * Uses map-based per-feature surface area accumulation with per-face area values
 * (computeFaceSurfaceAreas), matching Nathan Young's bug fix for correct surface
 * area computation across faces of different sizes.
 *
 * Surface feature detection handles 3D (all 6 boundary planes) and 2D (4 boundary
 * edges in the non-degenerate plane). 1D and 0D geometries are small enough to
 * always use the in-core Direct path, but are handled correctly here as well.
 */
Result<> ComputeFeatureNeighborsScanline::operator()()
{
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumberOfNeighborsPath)->getDataStoreRef();

  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->SharedSurfaceAreaListPath);

  auto* boundaryCellsStore = m_InputValues->StoreBoundaryCells ? m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore() : nullptr;
  auto* surfaceFeatures = m_InputValues->StoreSurfaceFeatures ? m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore() : nullptr;

  usize totalFeatures = numNeighbors.getNumberOfTuples();

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  const int64 dimX = static_cast<int64>(uDims[0]);
  const int64 dimY = static_cast<int64>(uDims[1]);
  const int64 dimZ = static_cast<int64>(uDims[2]);
  const usize sliceSize = static_cast<usize>(dimX) * static_cast<usize>(dimY);

  FloatVec3 spacing32 = imageGeom.getSpacing();
  std::array<float64, 3> spacing64 = {static_cast<float64>(spacing32[0]), static_cast<float64>(spacing32[1]), static_cast<float64>(spacing32[2])};

  // Per-face areas indexed by FaceNeighborType:
  // [0] = -Z face (XY plane), [1] = -Y face (XZ plane), [2] = -X face (YZ plane),
  // [3] = +X face (YZ plane), [4] = +Y face (XZ plane), [5] = +Z face (XY plane)
  const std::array<float64, 6> precomputedFaceAreas = computeFaceSurfaceAreas(spacing64);

  // Map-based accumulation: neighborSurfaceAreas[featureId][neighborFeatureId] = total shared area
  // This replaces the old vector-based counting + deduplication approach and fixes the
  // surface area calculation bug where all faces were assumed to have the same area.
  std::vector<std::map<usize, float64>> neighborSurfaceAreas(totalFeatures);

  // Max feature ID validation is deferred to after the slice loop
  // to avoid a separate full scan through OOC data. The loop's
  // `feature < totalFeatures` guard prevents out-of-bounds access.
  int32 observedMaxFeatureId = 0;

  // 3-slice rolling window for Z-sequential bulk I/O
  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);
  std::vector<int8> boundaryCellsSlice;
  if(boundaryCellsStore != nullptr)
  {
    boundaryCellsSlice.resize(sliceSize, 0);
  }

  // Load the first slice
  featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  // Load the second slice if it exists
  if(dimZ > 1)
  {
    featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  for(int64 z = 0; z < dimZ; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(boundaryCellsStore != nullptr)
    {
      std::fill(boundaryCellsSlice.begin(), boundaryCellsSlice.end(), static_cast<int8>(0));
    }

    for(int64 y = 0; y < dimY; y++)
    {
      const usize yStride = static_cast<usize>(y) * static_cast<usize>(dimX);

      for(int64 x = 0; x < dimX; x++)
      {
        const usize localIndex = yStride + static_cast<usize>(x);
        int8 numDiffNeighbors = 0;
        const int32 feature = curSlice[localIndex];

        if(feature > observedMaxFeatureId)
        {
          observedMaxFeatureId = feature;
        }

        if(feature > 0 && static_cast<usize>(feature) < totalFeatures)
        {
          // Surface feature detection: a feature is a surface feature if it
          // touches a boundary face in a non-degenerate dimension (size > 1).
          // Dimensions with size == 1 are "empty" and their boundary faces
          // do not count — except when ALL dimensions are degenerate (single
          // voxel), in which case the feature is trivially on the surface.
          // This matches Nathan Young's constexpr ImageDimensionState
          // handling in the Direct variant.
          if(surfaceFeatures != nullptr)
          {
            bool isBoundary = (dimX == 1 && dimY == 1 && dimZ == 1); // single voxel is trivially surface
            if(dimX > 1 && (x == 0 || x == dimX - 1))
            {
              isBoundary = true;
            }
            if(dimY > 1 && (y == 0 || y == dimY - 1))
            {
              isBoundary = true;
            }
            if(dimZ > 1 && (z == 0 || z == dimZ - 1))
            {
              isBoundary = true;
            }
            if(isBoundary)
            {
              surfaceFeatures->setValue(feature, true);
            }
          }

          // Check -Z neighbor (from previous slice buffer)
          if(z > 0)
          {
            const int32 neighborFeature = prevSlice[localIndex];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_NegativeZNeighbor];
            }
          }

          // Check -Y neighbor (within current slice)
          if(y > 0)
          {
            const int32 neighborFeature = curSlice[localIndex - static_cast<usize>(dimX)];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_NegativeYNeighbor];
            }
          }

          // Check -X neighbor (within current slice)
          if(x > 0)
          {
            const int32 neighborFeature = curSlice[localIndex - 1];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_NegativeXNeighbor];
            }
          }

          // Check +X neighbor (within current slice)
          if(x < dimX - 1)
          {
            const int32 neighborFeature = curSlice[localIndex + 1];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_PositiveXNeighbor];
            }
          }

          // Check +Y neighbor (within current slice)
          if(y < dimY - 1)
          {
            const int32 neighborFeature = curSlice[localIndex + static_cast<usize>(dimX)];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_PositiveYNeighbor];
            }
          }

          // Check +Z neighbor (from next slice buffer)
          if(z < dimZ - 1)
          {
            const int32 neighborFeature = nextSlice[localIndex];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[k_PositiveZNeighbor];
            }
          }
        }

        if(boundaryCellsStore != nullptr)
        {
          boundaryCellsSlice[localIndex] = numDiffNeighbors;
        }
      }
    }

    // Write the boundaryCells slice to the output store
    if(boundaryCellsStore != nullptr)
    {
      boundaryCellsStore->copyFromBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<const int8>(boundaryCellsSlice.data(), sliceSize));
    }

    // Rotate the rolling window
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(z + 2 < dimZ)
    {
      featureIds.copyIntoBuffer(static_cast<usize>(z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // Validate max feature ID (deferred from before the loop to avoid a separate OOC scan)
  if(static_cast<usize>(observedMaxFeatureId) >= totalFeatures)
  {
    return MakeErrorResult(-24500, fmt::format("Data Array {} has a maximum value of {} which is greater than the number of features from array {} which has {}. "
                                               "Did you select the incorrect array for the 'FeatureIds' array?",
                                               m_InputValues->FeatureIdsPath.getTargetName(), observedMaxFeatureId, m_InputValues->NumberOfNeighborsPath.getTargetName(), totalFeatures));
  }

  // Convert accumulated per-feature surface area maps to NeighborList objects.
  // Map keys are sorted by neighbor feature ID, matching the Direct variant's output order.
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    const usize neighborCount = neighborSurfaceAreas[featureIdx].size();
    numNeighbors.setValue(featureIdx, static_cast<int32>(neighborCount));

    auto sharedNeiLst = std::make_shared<NeighborList<int32>::VectorType>();
    sharedNeiLst->reserve(neighborCount);
    auto sharedSAL = std::make_shared<NeighborList<float32>::VectorType>();
    sharedSAL->reserve(neighborCount);
    for(const auto& [featureId, surfaceArea] : neighborSurfaceAreas[featureIdx])
    {
      sharedNeiLst->push_back(static_cast<int32>(featureId));
      sharedSAL->push_back(static_cast<float32>(surfaceArea));
    }
    neighborList.setList(static_cast<int32>(featureIdx), sharedNeiLst);
    sharedSurfaceAreaList.setList(static_cast<int32>(featureIdx), sharedSAL);
  }

  return {};
}
