#include "ComputeFeatureNeighborsDirect.hpp"

#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// =============================================================================
// ComputeFeatureNeighborsDirect — In-Core Algorithm
//
// This file implements the in-core (Direct) variant of ComputeFeatureNeighbors.
// It is selected by DispatchAlgorithm when all input arrays reside in memory.
//
// ALGORITHM OVERVIEW:
//   For each voxel in an ImageGeom, compare its FeatureId against the FeatureIds
//   of its 6 face neighbors (+/-X, +/-Y, +/-Z). When two adjacent voxels belong
//   to different features, accumulate the shared face's surface area into a
//   per-feature-pair map. After all voxels are processed, convert the maps into
//   NeighborList and SharedSurfaceAreaList arrays.
//
// KEY DESIGN DECISIONS:
//   1. Compile-time dimension specialization via ImageDimensionState<> templates
//      eliminates runtime branching for degenerate dimensions (1D, 2D geometries).
//   2. Two-stage processing separates boundary cells (which need validity checks)
//      from internal cells (where all 6 neighbors are guaranteed valid), removing
//      a branch from the innermost loop of Stage 2.
//   3. Per-face surface areas use precomputed values from computeFaceSurfaceAreas()
//      rather than a uniform area, fixing a DREAM3D 6.5 bug.
//
// DATA ACCESS PATTERN:
//   Uses getValue() for per-element random access. This is optimal for in-memory
//   DataStore where getValue() is essentially a pointer dereference.
// =============================================================================

namespace
{
// =============================================================================
// ImageDimensionState — compile-time specialization for image geometry variants.
//
// Encodes which dimensions have only 1 cell ("empty") to enable constexpr
// elimination of boundary processing code for degenerate dimensions.
// Authored by Nathan Young as part of the ComputeFeatureNeighbors rewrite.
// =============================================================================
template <bool EmptyXV, bool EmptyYV, bool EmptyZV>
struct ImageDimensionState
{
  static constexpr bool HasEmptyXDim = EmptyXV;
  static constexpr bool HasEmptyYDim = EmptyYV;
  static constexpr bool HasEmptyZDim = EmptyZV;

  static constexpr bool Is1DImageDimsState()
  {
    return (HasEmptyXDim == true && HasEmptyYDim == true && HasEmptyZDim == false) || (HasEmptyXDim == true && HasEmptyYDim == false && HasEmptyZDim == true) ||
           (HasEmptyXDim == false && HasEmptyYDim == true && HasEmptyZDim == true);
  }

  static constexpr bool Is2DImageDimsState()
  {
    return (HasEmptyXDim == true && HasEmptyYDim == false && HasEmptyZDim == false) || (HasEmptyXDim == false && HasEmptyYDim == true && HasEmptyZDim == false) ||
           (HasEmptyXDim == false && HasEmptyYDim == false && HasEmptyZDim == true);
  }
};

using Image3D = ImageDimensionState<false, false, false>;
using EmptyXImage2D = ImageDimensionState<true, false, false>;
using EmptyYImage2D = ImageDimensionState<false, true, false>;
using EmptyZImage2D = ImageDimensionState<false, false, true>;
using XImage1D = ImageDimensionState<false, true, true>;
using YImage1D = ImageDimensionState<true, false, true>;
using ZImage1D = ImageDimensionState<true, true, false>;
using SingleVoxelImage = ImageDimensionState<true, true, true>;

template <class ActualT, class ExpectedT>
constexpr bool IsExpectedImageDimsState()
{
  return ActualT::HasEmptyXDim == ExpectedT::HasEmptyXDim && ActualT::HasEmptyYDim == ExpectedT::HasEmptyYDim && ActualT::HasEmptyZDim == ExpectedT::HasEmptyZDim;
}

// =============================================================================
// ComputeFeatureNeighborsFunctor — core algorithm functor.
//
// Template parameters:
//   ProcessSurfaceFeaturesV — whether to populate the SurfaceFeatures array
//   ProcessBoundaryCellsV   — whether to populate the BoundaryCells array
//
// Uses two-stage processing to minimize runtime branching in the innermost loop:
//   Stage 1: Boundary cells (corners, edges, faces) — with validity checks
//   Stage 2: Internal cells (3D only) — all 6 neighbors guaranteed valid
//
// Surface area accumulation uses per-face area values (computeFaceSurfaceAreas)
// instead of a uniform area, fixing a bug present in DREAM3D 6.5 where all
// faces were assumed to have the same area as the XY face.
//
// Authored by Nathan Young and Jared Duffey.
// =============================================================================
template <bool ProcessSurfaceFeaturesV, bool ProcessBoundaryCellsV>
struct ComputeFeatureNeighborsFunctor
{
  template <class ImageDimensionStateT>
  Result<> operator()(BoolAbstractDataStore* surfaceFeatures, Int8AbstractDataStore* boundaryCells, Float32NeighborList& sharedSurfaceAreaList, Int32NeighborList& neighborsList,
                      Int32AbstractDataStore& numNeighbors, const Int32AbstractDataStore& featureIds, usize totalFeatures, const std::array<int64, 3>& dims, const std::array<float64, 3> spacing,
                      const std::array<int64, 6>& neighborVoxelIndexOffsets, const std::atomic_bool& shouldCancel) const
  {
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

    const usize totalPoints = featureIds.getNumberOfTuples();

    const std::array<float64, 6> precomputedFaceAreas = computeFaceSurfaceAreas(spacing);
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
     * `processFaceCell`, the main difference between the two being that the "Frame" algorithm
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
    const std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();
    const auto processFrameCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex) -> void {
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
        std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIndex, yIndex, zIndex, dims);
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

    // Process Corners
    {
      /**
       * Process Corners:
       * The constexpr logic in the code block will handle the following, using XYZ indexes:
       *
       * Case 1: Empty X
       *  - 0,0,0
       *  - 0,n_Y,0
       *  - 0,0,n_Z
       *  - 0,n_Y,n_Z
       *
       * Case 2: Empty Y
       *  - 0,0,0
       *  - n_X,0,0
       *  - 0,0,n_Z
       *  - n_X,0,n_Z
       *
       * Case 3: Empty Z
       *  - 0,0,0
       *  - n_X,0,0
       *  - 0,n_Y,0
       *  - n_X,n_Y,0
       *
       * Case 4: 3D Image (Image Stack)
       * - 0,0,0
       * - n_X,0,0
       * - 0,n_Y,0
       * - 0,0,n_Z
       * - n_X,n_Y,0
       * - n_X,0,n_Z
       * - 0,n_Y,n_Z
       * - n_X,n_Y,n_Z
       */

      processFrameCell(0, 0, 0);
      if constexpr(ProcessSurfaceFeaturesV && ImageDimensionStateT::Is1DImageDimsState())
      {
        // Since the frame cell function is shared between corners and edges
        // 1D case for border feature flagging must be disabled to prevent
        // the entire row from being flagged, thus we must do the corners
        // in an explicit action.
        // Note here that there is an argument that a new function
        // should be defined to handle corner cells. However, this was
        // decided against to avoid needles code duplication, as the
        // difference between the two functions would be a single
        // constexpr if statement
        const int32 feature = featureIds.getValue(0);
        surfaceFeatures->setValue(feature, true);
      }
      if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, SingleVoxelImage>())
      {
        processFrameCell(dims[2] - 1, dims[1] - 1, dims[0] - 1); // If 2D the dims in empty dimension is 1 so this line effectively preforms for all cases

        if constexpr(ProcessSurfaceFeaturesV && ImageDimensionStateT::Is1DImageDimsState())
        {
          // Since the frame cell function is shared between corners and edges
          // 1D case for border feature flagging must be disabled to prevent
          // the entire row from being flagged, thus we must do the corners
          // in an explicit action.
          // Note here that there is an argument that a new function
          // should be defined to handle corner cells. However, this was
          // decided against to avoid needles code duplication, as the
          // difference between the two functions would be a single
          // constexpr if statement
          const int64 voxelIndex = (dims[0] * dims[1] * (dims[2] - 1)) + (dims[0] * (dims[1] - 1)) + (dims[0] - 1);
          const int32 feature = featureIds.getValue(voxelIndex);
          surfaceFeatures->setValue(feature, true);
        }

        if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
        {
          if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyXImage2D>())
          {
            processFrameCell(0, 0, dims[0] - 1);
          }
          if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyYImage2D>())
          {
            processFrameCell(0, dims[1] - 1, 0);
          }
          if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyZImage2D>())
          {
            processFrameCell(dims[2] - 1, 0, 0);
          }
          if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
          {
            processFrameCell(0, dims[1] - 1, dims[0] - 1);
            processFrameCell(dims[2] - 1, 0, dims[0] - 1);
            processFrameCell(dims[2] - 1, dims[1] - 1, 0);
          }
        }
      }
    }

    // Case 0: Process Edges
    // X Edges
    if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyXImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, XImage1D>() ||
                 IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
    {
      for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
      {
        processFrameCell(0, 0, xIndex);
        if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
        {
          if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
          {
            processFrameCell(0, dims[1] - 1, xIndex);
            processFrameCell(dims[2] - 1, 0, xIndex);
          }
          processFrameCell(dims[2] - 1, dims[1] - 1, xIndex);
        }
      }
    }

    // Y Edges
    if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyYImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, YImage1D>() ||
                 IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
    {
      for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
      {
        processFrameCell(0, yIndex, 0);
        if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
        {
          if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
          {
            processFrameCell(0, yIndex, dims[0] - 1);
            processFrameCell(dims[2] - 1, yIndex, 0);
          }
          processFrameCell(dims[2] - 1, yIndex, dims[0] - 1);
        }
      }
    }

    // Z Edges
    if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyZImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, ZImage1D>() ||
                 IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
    {
      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        processFrameCell(zIndex, 0, 0);
        if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
        {
          if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
          {
            processFrameCell(zIndex, 0, dims[0] - 1);
            processFrameCell(zIndex, dims[1] - 1, 0);
          }
          processFrameCell(zIndex, dims[1] - 1, dims[0] - 1);
        }
      }
    }

    // Process Planes for 2D and 3D (Stack) Images
    if constexpr(!ImageDimensionStateT::Is1DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, SingleVoxelImage>())
    {
      const auto processFaceCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex, const std::vector<FaceNeighborType>& validFaces) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV && IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
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

      // Case 1: Z Planes
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          std::vector<FaceNeighborType> negZValidFaces = {k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
          std::vector<FaceNeighborType> posZValidFaces = {k_NegativeZNeighbor, k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor};
          for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
          {
            for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
            {
              processFaceCell(0, yIndex, xIndex, negZValidFaces);
              processFaceCell(dims[2] - 1, yIndex, xIndex, posZValidFaces);
            }
          }
        }
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, EmptyZImage2D>())
        {
          std::vector<FaceNeighborType> validFaces = {k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor};
          for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
          {
            for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
            {
              processFaceCell(0, yIndex, xIndex, validFaces);
            }
          }
        }
      }

      // Case 2: Y Planes
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          std::vector<FaceNeighborType> negYValidFaces = {k_NegativeZNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
          std::vector<FaceNeighborType> posYValidFaces = {k_NegativeZNeighbor, k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveZNeighbor};
          for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
          {
            for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
            {
              processFaceCell(zIndex, 0, xIndex, negYValidFaces);
              processFaceCell(zIndex, dims[1] - 1, xIndex, posYValidFaces);
            }
          }
        }
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, EmptyYImage2D>())
        {
          std::vector<FaceNeighborType> validFaces = {k_NegativeZNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveZNeighbor};
          for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
          {
            for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
            {
              processFaceCell(zIndex, 0, xIndex, validFaces);
            }
          }
        }
      }

      // Case 3: X Planes
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          std::vector<FaceNeighborType> negXValidFaces = {k_NegativeZNeighbor, k_NegativeYNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
          std::vector<FaceNeighborType> posXValidFaces = {k_NegativeZNeighbor, k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
          for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
          {
            for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
            {
              processFaceCell(zIndex, yIndex, 0, negXValidFaces);
              processFaceCell(zIndex, yIndex, dims[0] - 1, posXValidFaces);
            }
          }
        }
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, EmptyXImage2D>())
        {
          std::vector<FaceNeighborType> validFaces = {k_NegativeZNeighbor, k_NegativeYNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
          for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
          {
            for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
            {
              processFaceCell(zIndex, yIndex, 0, validFaces);
            }
          }
        }
      }
    }

    /**
     * Stage 2: Process Internal Cells
     * This stage has a bulk of the computation, and runtime branching has been minimized
     * to reflect that reality, see comment for Stage 1. This section just walks every
     * internal cell and checks each of the neighbors, storing them onto the existing
     * results from the boundary cell phases.
     */
    if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
    {
      // Loop over all internal cells to generate the neighbor lists
      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        const int64 zStride = dims[0] * dims[1] * zIndex;
        for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
        {
          const int64 yStride = dims[0] * yIndex;
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
ComputeFeatureNeighborsDirect::ComputeFeatureNeighborsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             const ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighborsDirect::~ComputeFeatureNeighborsDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief In-core implementation of ComputeFeatureNeighbors.
 *
 * Uses Nathan Young's rewritten algorithm with compile-time dimension specialization
 * and per-face surface area accumulation.
 *
 * Accesses FeatureIds via getValue() (per-element random access), which is optimal
 * for in-memory DataStore where it is essentially a pointer dereference. For OOC
 * data, the Scanline variant reads Z-slices via bulk I/O instead.
 *
 * The function dispatches to one of 4 template specializations based on boolean
 * combinations of StoreSurfaceFeatures and StoreBoundaryCells, eliminating those
 * branches from the innermost loop via constexpr if.
 */
Result<> ComputeFeatureNeighborsDirect::operator()()
{
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
    out << "Data Array " << m_InputValues->FeatureIdsPath.getTargetName() << " has a maximum value of " << maxFeatureId << " which is greater than the "
        << " number of features from array " << m_InputValues->NumberOfNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {static_cast<int64>(uDims[0]), static_cast<int64>(uDims[1]), static_cast<int64>(uDims[2])};

  FloatVec3 spacing32 = imageGeom.getSpacing();

  std::array<float64, 3> spacing64 = {static_cast<float64>(spacing32[0]), static_cast<float64>(spacing32[1]), static_cast<float64>(spacing32[2])};

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);

  if(m_InputValues->StoreSurfaceFeatures && m_InputValues->StoreBoundaryCells)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, true>{}, imageGeom, surfaceFeatures, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, neighborVoxelIndexOffsets, m_ShouldCancel);
  }
  if(m_InputValues->StoreSurfaceFeatures)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, false>{}, imageGeom, surfaceFeatures, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, neighborVoxelIndexOffsets, m_ShouldCancel);
  }
  if(m_InputValues->StoreBoundaryCells)
  {
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, true>{}, imageGeom, nullptr, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, neighborVoxelIndexOffsets, m_ShouldCancel);
  }

  return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, false>{}, imageGeom, nullptr, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims, spacing64,
                       neighborVoxelIndexOffsets, m_ShouldCancel);
}
