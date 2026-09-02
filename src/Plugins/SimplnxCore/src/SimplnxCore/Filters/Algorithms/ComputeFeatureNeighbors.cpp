#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

namespace
{
template <bool ProcessSurfaceFeaturesV, bool ProcessBoundaryCellsV>
struct ComputeFeatureNeighborsFunctor
{
  template <detail::ImageDimensionality ImageDimensionStateT>
  Result<> operator()(BoolAbstractDataStore* surfaceFeatures, Int8AbstractDataStore* boundaryCells, Float32NeighborList& sharedSurfaceAreaList, Int32NeighborList& neighborsList,
                      Int32AbstractDataStore& numNeighbors, const Int32AbstractDataStore& featureIds, usize totalFeatures, const std::array<int64, 3>& dims, const std::array<float64, 3> spacing,
                      ThrottledMessageHandler& throttledMessenger, const std::atomic_bool& shouldCancel) const
  {
    constexpr FaceNeighborType k_NeighborCount = VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount;

    if(ProcessSurfaceFeaturesV)
    {
      if(surfaceFeatures == nullptr)
      {
        return MakeErrorResult(-789620, "Process Surface Features selected, but the supplied Surface Features Array invalid.");
      }
    }

    if(ProcessBoundaryCellsV)
    {
      if(boundaryCells == nullptr)
      {
        return MakeErrorResult(-789621, "Process Boundary Cells selected, but the supplied Boundary Cells Array invalid.");
      }
    }
    const std::array<int64, k_NeighborCount> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets<ImageDimensionStateT>(dims);

    const std::array<float64, k_NeighborCount> precomputedFaceAreas = computeFaceSurfaceAreas<ImageDimensionStateT>(spacing);
    std::vector<std::map<usize, float64>> neighborSurfaceAreas(totalFeatures);

    /**
     * Stage 1: Process Boundary Cells
     *
     * The primary goal of Stage 1 is to isolate border cell specific checks out of Phase 2
     * (the internal cells). This includes flagging the border cells without needing to
     * branch, and ignoring invalid voxel faces inherently as much as possible. This segmentation
     * also allows for removing a branch in the deepest nested loop in Phase 2.
     *
     * Stage 1 has been split into 3 parts, the vertex (corner), edge, and face cells.
     * Of these parts there are two main logic flows defined by `processFrameCell` and
     * `processFaceCell`, the main difference between the two being that the "Frame" algorithms
     * (corner and edge are nearly identical minus one dimension case so they are grouped as "Frame")
     * checks every face neighbor and validates them, whereas the "Face" algorithm removes the
     * validation check and cuts down the checked faces to only the valid ones. It should also be
     * noted that, optimization is being left on the table with the frame section. It could be further
     * broken down into processing each edge/voxel individually to mirror the optimization done to
     * faces, but the segmentation done here would make it far less readable and in the greater context
     * the speed gain is minimal considering they are O(n-2) and O(1) respectively and the greater algorithm
     * is 0(6(n-2)^3).
     *
     * Note here that discussions were had of adding Kahan Summation for calculating the surface
     * areas, but was decided against to conserve memory. At least until the issue presents itself
     * in a real world dataset.
     */
    constexpr std::array<FaceNeighborType, k_NeighborCount> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx<ImageDimensionStateT>();

    // Process Corners
    {
      const auto processCornerCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV)
          {
            surfaceFeatures->setValue(feature, true);
          }

          // Loop over the 6 face neighbors of the voxel
          std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimensionStateT>(xIndex, yIndex, zIndex, dims);
          for(const auto faceIndex : faceNeighborInternalIdx) // ref more expensive than trivial copy for scalar types
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessCorners<ImageDimensionStateT>(processCornerCell, dims);
    }

    // Process Edges
    if constexpr(!std::is_same_v<ImageDimensionStateT, SingleVoxelImage>)
    {
      const auto processEdgeCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV && !ImageDimensionStateT::Is1DImageDimsState())
          {
            surfaceFeatures->setValue(feature, true);
          }

          // Loop over the 6 face neighbors of the voxel
          std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimensionStateT>(xIndex, yIndex, zIndex, dims);
          for(const auto faceIndex : faceNeighborInternalIdx) // ref more expensive than trivial copy for scalar types
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessEdges<ImageDimensionStateT>(processEdgeCell, dims);
    }

    // Process Planes for 2D and 3D (Stack) Images
    if constexpr(!ImageDimensionStateT::Is1DImageDimsState() && !std::is_same_v<ImageDimensionStateT, SingleVoxelImage>)
    {
      const auto processFaceCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex, const std::vector<FaceNeighborType>& validFaces) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV && std::is_same_v<ImageDimensionStateT, Image3D>)
          {
            surfaceFeatures->setValue(feature, true);
          }

          // Loop over the face neighbors of the voxel
          for(const auto faceIndex : validFaces) // ref more expensive than trivial copy for scalar types
          {
            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessFaces<ImageDimensionStateT>(processFaceCell, dims);
    }

    /**
     * Stage 2: Process Internal Cells
     * This stage has a bulk of the computation, and runtime branching has been minimized
     * to reflect that reality, see comment for Stage 1. This section just walks every
     * internal cell and checks each of the neighbors, storing them onto the existing
     * results from the boundary cell phases.
     */
    if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
    {
      const usize totalPoints = featureIds.getNumberOfTuples();

      // Loop over all internal cells to generate the neighbor lists
      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        const int64 zStride = dims[0] * dims[1] * zIndex;
        for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
        {
          const int64 yStride = dims[0] * yIndex;
          throttledMessenger.updatePercent("Determining Neighbor Lists", zStride + yStride, totalPoints);

          if(shouldCancel)
          {
            return {};
          }
          for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
          {
            int64 voxelIndex = zStride + yStride + xIndex;

            // This value tracks the number of neighboring cells that have feature ids different from itself
            int8 numDiffNeighbors = 0;
            int32 feature = featureIds.getValue(voxelIndex);
            if(feature > 0)
            {
              // Loop over the face neighbors of the voxel
              for(const auto faceIndex : faceNeighborInternalIdx) // ref more expensive than trivial copy for scalar types
              {
                // No need for a face validity check because we are only processing internal cells

                const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

                const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
                if(neighborFeatureId != feature && neighborFeatureId > 0)
                {
                  numDiffNeighbors++;
                  neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
                }
              }
            }
            if constexpr(ProcessBoundaryCellsV)
            {
              boundaryCells->setValue(voxelIndex, numDiffNeighbors);
            }
          }
        }
      }
    }

    for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
    {
      const usize neighborCount = neighborSurfaceAreas[featureIdx].size();
      numNeighbors.setValue(featureIdx, static_cast<int32>(neighborCount));

      // Set the vector for each list into the NeighborList Object
      auto sharedNeiLst = std::make_shared<NeighborList<int32>::VectorType>();
      sharedNeiLst->reserve(neighborCount);
      auto sharedSAL = std::make_shared<NeighborList<float32>::VectorType>();
      sharedSAL->reserve(neighborCount);
      for(const auto& [featureId, surfaceArea] : neighborSurfaceAreas[featureIdx])
      {
        sharedNeiLst->push_back(static_cast<int32>(featureId));
        sharedSAL->push_back(static_cast<float32>(surfaceArea));
      }
      neighborsList.setList(static_cast<int32>(featureIdx), sharedNeiLst);
      sharedSurfaceAreaList.setList(static_cast<int32>(featureIdx), sharedSAL);
    }

    return {};
  }
};

template <class FunctorT, class... ArgsT>
Result<> ProcessVoxels(const FunctorT& functor, const ImageGeom& imageGeom, ArgsT&&... args)
{
  const bool xDimEmpty = imageGeom.getNumXCells() == 1;
  const bool yDimEmpty = imageGeom.getNumYCells() == 1;
  const bool zDimEmpty = imageGeom.getNumZCells() == 1;
  const uint8 emptyDimCount = static_cast<uint8>(xDimEmpty) + static_cast<uint8>(yDimEmpty) + static_cast<uint8>(zDimEmpty);

  // Treat dimensions of 1 as flat for image geom
  if(emptyDimCount == 0)
  {
    return functor.template operator()<Image3D>(std::forward<ArgsT>(args)...);
  }
  if(emptyDimCount == 1)
  {
    if(zDimEmpty)
    {
      return functor.template operator()<EmptyZImage2D>(std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty)
    {
      return functor.template operator()<EmptyYImage2D>(std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty)
    {
      return functor.template operator()<EmptyXImage2D>(std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 2)
  {
    if(xDimEmpty && yDimEmpty)
    {
      return functor.template operator()<ZImage1D>(std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty && zDimEmpty)
    {
      return functor.template operator()<YImage1D>(std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty && zDimEmpty)
    {
      return functor.template operator()<XImage1D>(std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 3)
  {
    return functor.template operator()<SingleVoxelImage>(std::forward<ArgsT>(args)...);
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::~ComputeFeatureNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighbors::operator()()
{
  ThrottledMessageHandler throttledMessenger(m_MessageHandler);

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumberOfNeighborsPath)->getDataStoreRef();

  auto& neighborsList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->SharedSurfaceAreaListPath);

  usize totalFeatures = numNeighbors.getNumberOfTuples();

  /* Ensure that we will be able to work with the user selected featureId Array */
  const int32 maxFeatureId = *std::max_element(featureIds.cbegin(), featureIds.cend());
  if(static_cast<usize>(maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << m_InputValues->FeatureIdsPath.getTargetName() << " has a maximum value of " << maxFeatureId << " which is greater than the " << " number of features from array "
        << m_InputValues->NumberOfNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the " << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {static_cast<int64>(uDims[0]), static_cast<int64>(uDims[1]), static_cast<int64>(uDims[2])};

  FloatVec3 spacing32 = imageGeom.getSpacing();

  std::array<float64, 3> spacing64 = {static_cast<float64>(spacing32[0]), static_cast<float64>(spacing32[1]), static_cast<float64>(spacing32[2])};

  if(m_InputValues->StoreSurfaceFeatures && m_InputValues->StoreBoundaryCells)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, true>{}, imageGeom, surfaceFeatures, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }
  if(m_InputValues->StoreSurfaceFeatures)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, false>{}, imageGeom, surfaceFeatures, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }
  if(m_InputValues->StoreBoundaryCells)
  {
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, true>{}, imageGeom, nullptr, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }

  return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, false>{}, imageGeom, nullptr, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims, spacing64,
                       throttledMessenger, m_ShouldCancel);
}
