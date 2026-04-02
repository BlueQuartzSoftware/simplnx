#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

namespace nx::core::ImageDimensionalUtilities
{
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

template <typename FT>
concept FrameCellFunction = requires(FT f, int64 x, int64 y, int64 z) {
                              {
                                f(x, y, z)
                                } -> std::same_as<void>;
                            };

template <typename FT>
concept FaceCellFunction = requires(FT f, int64 x, int64 y, int64 z, const std::vector<FaceNeighborType>& validNeighbors) {
                             {
                               f(x, y, z, validNeighbors)
                               } -> std::same_as<void>;
                           };

/**
 * Process Corners:
 * The constexpr logic in the code block will handle the following, using XYZ indexes:
 * Case 0: Single Voxel
 *  - 0,0,0
 *
 * Case 1: Only X
 *  - 0,0,0
 *  - n_X,0,0
 *
 * Case 2: Only Y
 *  - 0,0,0
 *  - 0,n_Y,0
 *
 * Case 3: Only Z
 *  - 0,0,0
 *  - 0,0,n_Z
 *
 * Case 4: Empty X
 *  - 0,0,0
 *  - 0,n_Y,0
 *  - 0,0,n_Z
 *  - 0,n_Y,n_Z
 *
 * Case 5: Empty Y
 *  - 0,0,0
 *  - n_X,0,0
 *  - 0,0,n_Z
 *  - n_X,0,n_Z
 *
 * Case 6: Empty Z
 *  - 0,0,0
 *  - n_X,0,0
 *  - 0,n_Y,0
 *  - n_X,n_Y,0
 *
 * Case 7: 3D Image (Image Stack)
 * - 0,0,0
 * - n_X,0,0
 * - 0,n_Y,0
 * - 0,0,n_Z
 * - n_X,n_Y,0
 * - n_X,0,n_Z
 * - 0,n_Y,n_Z
 * - n_X,n_Y,n_Z
 */
template <class ImageDimensionStateT, FrameCellFunction FunctionT>
void ProcessCorners(const FunctionT& processCornerFunction, const std::array<int64, 3>& dims)
{
  processCornerFunction(0, 0, 0);
  if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, SingleVoxelImage>())
  {
    processCornerFunction(dims[2] - 1, dims[1] - 1, dims[0] - 1); // If 2D the dims in empty dimension is 1 so this line effectively preforms for all cases

    if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
    {
      if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyXImage2D>())
      {
        processCornerFunction(0, 0, dims[0] - 1);
      }
      if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyYImage2D>())
      {
        processCornerFunction(0, dims[1] - 1, 0);
      }
      if constexpr(!IsExpectedImageDimsState<ImageDimensionStateT, EmptyZImage2D>())
      {
        processCornerFunction(dims[2] - 1, 0, 0);
      }
      if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
      {
        processCornerFunction(0, dims[1] - 1, dims[0] - 1);
        processCornerFunction(dims[2] - 1, 0, dims[0] - 1);
        processCornerFunction(dims[2] - 1, dims[1] - 1, 0);
      }
    }
  }
}

template <class ImageDimensionStateT, FrameCellFunction FunctionT>
void ProcessEdges(const FunctionT& processEdgeFunction, const std::array<int64, 3>& dims)
{
  static_assert(!IsExpectedImageDimsState<ImageDimensionStateT, SingleVoxelImage>());
  // X Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyXImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, XImage1D>() ||
               IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
  {
    for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
    {
      processEdgeFunction(0, 0, xIndex);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          processEdgeFunction(0, dims[1] - 1, xIndex);
          processEdgeFunction(dims[2] - 1, 0, xIndex);
        }
        processEdgeFunction(dims[2] - 1, dims[1] - 1, xIndex);
      }
    }
  }

  // Y Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyYImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, YImage1D>() ||
               IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
  {
    for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
    {
      processEdgeFunction(0, yIndex, 0);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          processEdgeFunction(0, yIndex, dims[0] - 1);
          processEdgeFunction(dims[2] - 1, yIndex, 0);
        }
        processEdgeFunction(dims[2] - 1, yIndex, dims[0] - 1);
      }
    }
  }

  // Z Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, EmptyZImage2D>()) || IsExpectedImageDimsState<ImageDimensionStateT, ZImage1D>() ||
               IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
  {
    for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
    {
      processEdgeFunction(zIndex, 0, 0);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(IsExpectedImageDimsState<ImageDimensionStateT, Image3D>())
        {
          processEdgeFunction(zIndex, 0, dims[0] - 1);
          processEdgeFunction(zIndex, dims[1] - 1, 0);
        }
        processEdgeFunction(zIndex, dims[1] - 1, dims[0] - 1);
      }
    }
  }
}

template <class ImageDimensionStateT, FaceCellFunction FunctionT>
void ProcessFaces(const FunctionT& processFaceFunction, const std::array<int64, 3>& dims)
{
  static_assert(!ImageDimensionStateT::Is1DImageDimsState() && !IsExpectedImageDimsState<ImageDimensionStateT, SingleVoxelImage>());
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
          processFaceFunction(0, yIndex, xIndex, negZValidFaces);
          processFaceFunction(dims[2] - 1, yIndex, xIndex, posZValidFaces);
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
          processFaceFunction(0, yIndex, xIndex, validFaces);
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
          processFaceFunction(zIndex, 0, xIndex, negYValidFaces);
          processFaceFunction(zIndex, dims[1] - 1, xIndex, posYValidFaces);
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
          processFaceFunction(zIndex, 0, xIndex, validFaces);
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
          processFaceFunction(zIndex, yIndex, 0, negXValidFaces);
          processFaceFunction(zIndex, yIndex, dims[0] - 1, posXValidFaces);
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
          processFaceFunction(zIndex, yIndex, 0, validFaces);
        }
      }
    }
  }
}
} // namespace nx::core::ImageDimensionalUtilities