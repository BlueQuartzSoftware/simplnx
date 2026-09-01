#include "ComputeFeatureNeighborsScanline.hpp"

#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <nonstd/span.hpp>

using namespace nx::core;

ComputeFeatureNeighborsScanline::ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 const ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureNeighborsScanline::~ComputeFeatureNeighborsScanline() noexcept = default;

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

  // Each face orientation uses its physical ImageGeom area.
  const std::array<float64, VoxelNeighbors<Image3D>::k_FaceNeighborCount> precomputedFaceAreas = computeFaceSurfaceAreas<Image3D>(spacing64);

  // Map keys accumulate every shared face under the neighboring Feature ID.
  std::vector<std::map<usize, float64>> neighborSurfaceAreas(totalFeatures);

  // The loop tracks the maximum ID to avoid a second full-volume scan and guards feature output indexing.
  int32 observedMaxFeatureId = 0;

  // Three slices resolve Z neighbors without random Feature ID store access.
  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);
  std::vector<int8> boundaryCellsSlice;
  if(boundaryCellsStore != nullptr)
  {
    boundaryCellsSlice.resize(sliceSize, 0);
  }

  featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
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
          // A feature is on the surface at a non-degenerate geometry face. A single voxel is surface.
          if(surfaceFeatures != nullptr)
          {
            bool isBoundary = (dimX == 1 && dimY == 1 && dimZ == 1);
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

          if(z > 0)
          {
            const int32 neighborFeature = prevSlice[localIndex];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_NegativeZNeighbor];
            }
          }

          if(y > 0)
          {
            const int32 neighborFeature = curSlice[localIndex - static_cast<usize>(dimX)];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_NegativeYNeighbor];
            }
          }

          if(x > 0)
          {
            const int32 neighborFeature = curSlice[localIndex - 1];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_NegativeXNeighbor];
            }
          }

          if(x < dimX - 1)
          {
            const int32 neighborFeature = curSlice[localIndex + 1];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_PositiveXNeighbor];
            }
          }

          if(y < dimY - 1)
          {
            const int32 neighborFeature = curSlice[localIndex + static_cast<usize>(dimX)];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_PositiveYNeighbor];
            }
          }

          if(z < dimZ - 1)
          {
            const int32 neighborFeature = nextSlice[localIndex];
            if(neighborFeature != feature && neighborFeature > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeature] += precomputedFaceAreas[VoxelNeighbors<Image3D>::k_PositiveZNeighbor];
            }
          }
        }

        if(boundaryCellsStore != nullptr)
        {
          boundaryCellsSlice[localIndex] = numDiffNeighbors;
        }
      }
    }

    if(boundaryCellsStore != nullptr)
    {
      boundaryCellsStore->copyFromBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<const int8>(boundaryCellsSlice.data(), sliceSize));
    }

    // Swaps rotate owned slice buffers in O(1) before z+2 is read.
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(z + 2 < dimZ)
    {
      featureIds.copyIntoBuffer(static_cast<usize>(z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  if(static_cast<usize>(observedMaxFeatureId) >= totalFeatures)
  {
    return MakeErrorResult(-24500, fmt::format("Data Array {} has a maximum value of {} which is greater than the number of features from array {} which has {}. "
                                               "Did you select the incorrect array for the 'FeatureIds' array?",
                                               m_InputValues->FeatureIdsPath.getTargetName(), observedMaxFeatureId, m_InputValues->NumberOfNeighborsPath.getTargetName(), totalFeatures));
  }

  // Map order keeps neighbor lists sorted by Feature ID.
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
