#pragma once

#include "simplnx/Common/Types.hpp"

#include <array>
#include <vector>

/**
 * @brief This set of enumerations and functions help facilitate a consistent algorithm
 * by which the 6 face-neighbors of a cube are accessed without stepping out of
 * the virtual volume. There is some basic setup that needs to be done.
 *
 * @code

  // Get the Image Geometry and pull from that the dimensions and then convert
  // those dimensions into int64 values
  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  // Grab the pair of std::array variables that define how to calculate the face neighbors voxel index
  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

 *
 * @endcode
 *
 * There are 2 major algorithms to loop over the voxels. The first is with a basic
 * loop over every voxel like the following:
* @code
for(int64 voxelIndex = 0; voxelIndex < totalVoxels; ++voxelIndex)
 * @endcode
 *
 * Or the second form is that of a triple nested loop over the X, Y and Z Dimensions
* @code
for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    kStride = dims[0] * dims[1] * zIdx;
    for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
    {
      jStride = dims[0] * yIdx;
      for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
      {
      ....
* @endcode
 *
 * At some point in the loop the algorithm will need to determine if the index is a
 * valid index and not off the virtual volume. This is where the other functions come in:
* @code
// Loop over the 6 face neighbors of the voxel
std::array<bool, 6> isValidFaceNeighbor = preCalculateValidFaceNeighbor(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }
        neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
        ....
* @endcode
*
* Example Filters:
*
* First Method: NeighborOrientationCorrelation
* Second Method: RequiredMinimumNumNeighbors
*/
namespace nx::core
{
using FaceNeighborType = usize;

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

namespace detail
{
template <class T>
struct IsSpecializationOfIDS : std::false_type
{
};

template <bool EmptyXV, bool EmptyYV, bool EmptyZV>
struct IsSpecializationOfIDS<ImageDimensionState<EmptyXV, EmptyYV, EmptyZV>> : std::true_type
{
};

template <typename T>
concept ImageDimensionality = IsSpecializationOfIDS<T>::value;
} // namespace detail
template <detail::ImageDimensionality ImageDimensionStateT>
struct VoxelNeighbors
{
};

template <>
struct VoxelNeighbors<Image3D>
{
  static constexpr FaceNeighborType k_NegativeZNeighbor = 0;
  static constexpr FaceNeighborType k_NegativeYNeighbor = 1;
  static constexpr FaceNeighborType k_NegativeXNeighbor = 2;
  static constexpr FaceNeighborType k_PositiveXNeighbor = 3;
  static constexpr FaceNeighborType k_PositiveYNeighbor = 4;
  static constexpr FaceNeighborType k_PositiveZNeighbor = 5;
  static constexpr FaceNeighborType k_FaceNeighborCount = 6;
};

template <>
struct VoxelNeighbors<EmptyXImage2D>
{
  static constexpr FaceNeighborType k_NegativeZNeighbor = 0;
  static constexpr FaceNeighborType k_NegativeYNeighbor = 1;
  static constexpr FaceNeighborType k_PositiveYNeighbor = 2;
  static constexpr FaceNeighborType k_PositiveZNeighbor = 3;
  static constexpr FaceNeighborType k_FaceNeighborCount = 4;
};

template <>
struct VoxelNeighbors<EmptyYImage2D>
{
  static constexpr FaceNeighborType k_NegativeZNeighbor = 0;
  static constexpr FaceNeighborType k_NegativeXNeighbor = 1;
  static constexpr FaceNeighborType k_PositiveXNeighbor = 2;
  static constexpr FaceNeighborType k_PositiveZNeighbor = 3;
  static constexpr FaceNeighborType k_FaceNeighborCount = 4;
};

template <>
struct VoxelNeighbors<EmptyZImage2D>
{
  static constexpr FaceNeighborType k_NegativeYNeighbor = 0;
  static constexpr FaceNeighborType k_NegativeXNeighbor = 1;
  static constexpr FaceNeighborType k_PositiveXNeighbor = 2;
  static constexpr FaceNeighborType k_PositiveYNeighbor = 3;
  static constexpr FaceNeighborType k_FaceNeighborCount = 4;
};

template <>
struct VoxelNeighbors<XImage1D>
{
  static constexpr FaceNeighborType k_NegativeXNeighbor = 0;
  static constexpr FaceNeighborType k_PositiveXNeighbor = 1;
  static constexpr FaceNeighborType k_FaceNeighborCount = 2;
};

template <>
struct VoxelNeighbors<YImage1D>
{
  static constexpr FaceNeighborType k_NegativeYNeighbor = 0;
  static constexpr FaceNeighborType k_PositiveYNeighbor = 1;
  static constexpr FaceNeighborType k_FaceNeighborCount = 2;
};

template <>
struct VoxelNeighbors<ZImage1D>
{
  static constexpr FaceNeighborType k_NegativeZNeighbor = 0;
  static constexpr FaceNeighborType k_PositiveZNeighbor = 1;
  static constexpr FaceNeighborType k_FaceNeighborCount = 2;
};

template <>
struct VoxelNeighbors<SingleVoxelImage>
{
  static constexpr FaceNeighborType k_FaceNeighborCount = 0;
};

/**
 * @brief
 * @return
 */
template <detail::ImageDimensionality ImageDimensionStateT = Image3D>
constexpr std::array<FaceNeighborType, VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount> initializeFaceNeighborInternalIdx()
{
  using Neighbors = VoxelNeighbors<ImageDimensionStateT>;
  if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    return {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor,
            Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor, Neighbors::k_PositiveZNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
  {
    return {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_PositiveYNeighbor, Neighbors::k_PositiveZNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
  {
    return {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveZNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
  {
    return {Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, XImage1D>)
  {
    return {Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, YImage1D>)
  {
    return {Neighbors::k_NegativeYNeighbor, Neighbors::k_PositiveYNeighbor};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, ZImage1D>)
  {
    return {Neighbors::k_NegativeZNeighbor, Neighbors::k_PositiveZNeighbor};
  }

  // Single Voxel State
  return {};
}

/**
 * @brief Get the 6 face-connected neighbor offsets
 * @param dims Image geometry dimensions
 * @return Array of 6 neighbor offsets
 */
template <detail::ImageDimensionality ImageDimensionStateT = Image3D>
std::array<int64, VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount> initializeFaceNeighborOffsets(const std::array<int64, 3>& dims)
{
  if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    return {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
  {
    return {-dims[1], -1, 1, dims[1]};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
  {
    return {-dims[0], -1, 1, dims[0]};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
  {
    return {-dims[0], -1, 1, dims[0]};
  }
  if constexpr(ImageDimensionStateT::Is1DImageDimsState())
  {
    return {-1, 1};
  }

  // Single Voxel State
  return {};
}

/**
 * @brief Returns a boolean array of 6 elements that indicate if the given neighbor is within the bounds of the virtual volume
 * @param xIdx The x index (column) of the current voxel
 * @param yIdx The y index (row) of the current voxel
 * @param zIdx The z index (plane) of the current voxel
 * @param dims The dimensions of the virtual volume
 * @return
 */
template <detail::ImageDimensionality ImageDimensionStateT = Image3D>
std::array<bool, VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount> computeValidFaceNeighbors(int64 xIdx, int64 yIdx, int64 zIdx, const std::array<int64, 3>& dims)
{
  if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    return {zIdx > 0, yIdx > 0, xIdx > 0, xIdx < dims[0] - 1, yIdx < dims[1] - 1, zIdx < dims[2] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
  {
    return {zIdx > 0, yIdx > 0, yIdx < dims[1] - 1, zIdx < dims[2] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
  {
    return {zIdx > 0, xIdx > 0, xIdx < dims[0] - 1, zIdx < dims[2] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
  {
    return {yIdx > 0, xIdx > 0, xIdx < dims[0] - 1, yIdx < dims[1] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, XImage1D>)
  {
    return {xIdx > 0, xIdx < dims[0] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, YImage1D>)
  {
    return {yIdx > 0, yIdx < dims[1] - 1};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, ZImage1D>)
  {
    return {zIdx > 0, zIdx < dims[2] - 1};
  }

  // Single Voxel State
  return {};
}

/**
 * @brief Returns the surface area of each face of the voxel corresponding to the
 * initializeFaceNeighborInternalIdx() ordering
 * @param spacing The spacing of each voxel
 * @return
 */
template <detail::ImageDimensionality ImageDimensionStateT = Image3D>
std::array<float64, VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount> computeFaceSurfaceAreas(const std::array<float64, 3>& spacing)
{
  const auto zFace = spacing[0] * spacing[1];
  const auto yFace = spacing[0] * spacing[2];
  const auto xFace = spacing[1] * spacing[2];

  if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    return {zFace, yFace, xFace, xFace, yFace, zFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
  {
    return {zFace, yFace, yFace, zFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
  {
    return {zFace, xFace, xFace, zFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
  {
    return {yFace, xFace, xFace, yFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, XImage1D>)
  {
    return {xFace, xFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, YImage1D>)
  {
    return {yFace, yFace};
  }
  if constexpr(std::is_same_v<ImageDimensionStateT, ZImage1D>)
  {
    return {zFace, zFace};
  }

  // Single Voxel State
  return {};
}

namespace ImageDimensionalUtilities
{

template <typename FT>
concept FrameCellFunction = requires(FT f, int64 x, int64 y, int64 z) {
  { f(x, y, z) } -> std::same_as<void>;
};

template <typename FT>
concept FaceCellFunction = requires(FT f, int64 x, int64 y, int64 z, const std::vector<FaceNeighborType>& validNeighbors) {
  { f(x, y, z, validNeighbors) } -> std::same_as<void>;
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
  if constexpr(!std::is_same_v<ImageDimensionStateT, SingleVoxelImage>)
  {
    processCornerFunction(dims[2] - 1, dims[1] - 1, dims[0] - 1); // If 2D the dims in empty dimension is 1 so this line effectively preforms for all cases

    if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
    {
      if constexpr(!std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
      {
        processCornerFunction(0, 0, dims[0] - 1);
      }
      if constexpr(!std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
      {
        processCornerFunction(0, dims[1] - 1, 0);
      }
      if constexpr(!std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
      {
        processCornerFunction(dims[2] - 1, 0, 0);
      }
      if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
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
  static_assert(!std::is_same_v<ImageDimensionStateT, SingleVoxelImage>);
  // X Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !std::is_same_v<ImageDimensionStateT, EmptyXImage2D>) || std::is_same_v<ImageDimensionStateT, XImage1D> ||
               std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
    {
      processEdgeFunction(0, 0, xIndex);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
        {
          processEdgeFunction(0, dims[1] - 1, xIndex);
          processEdgeFunction(dims[2] - 1, 0, xIndex);
        }
        processEdgeFunction(dims[2] - 1, dims[1] - 1, xIndex);
      }
    }
  }

  // Y Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !std::is_same_v<ImageDimensionStateT, EmptyYImage2D>) || std::is_same_v<ImageDimensionStateT, YImage1D> ||
               std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
    {
      processEdgeFunction(0, yIndex, 0);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
        {
          processEdgeFunction(0, yIndex, dims[0] - 1);
          processEdgeFunction(dims[2] - 1, yIndex, 0);
        }
        processEdgeFunction(dims[2] - 1, yIndex, dims[0] - 1);
      }
    }
  }

  // Z Edges
  if constexpr((ImageDimensionStateT::Is2DImageDimsState() && !std::is_same_v<ImageDimensionStateT, EmptyZImage2D>) || std::is_same_v<ImageDimensionStateT, ZImage1D> ||
               std::is_same_v<ImageDimensionStateT, Image3D>)
  {
    for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
    {
      processEdgeFunction(zIndex, 0, 0);
      if constexpr(!ImageDimensionStateT::Is1DImageDimsState())
      {
        if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
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
  static_assert(!ImageDimensionStateT::Is1DImageDimsState() && !std::is_same_v<ImageDimensionStateT, SingleVoxelImage>);
  using Neighbors = VoxelNeighbors<ImageDimensionStateT>;
  // Case 1: Z Planes
  {
    if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
    {
      std::vector<FaceNeighborType> negZValidFaces = {Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor,
                                                      Neighbors::k_PositiveZNeighbor};
      std::vector<FaceNeighborType> posZValidFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor,
                                                      Neighbors::k_PositiveYNeighbor};
      for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
      {
        for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
        {
          processFaceFunction(0, yIndex, xIndex, negZValidFaces);
          processFaceFunction(dims[2] - 1, yIndex, xIndex, posZValidFaces);
        }
      }
    }
    if constexpr(std::is_same_v<ImageDimensionStateT, EmptyZImage2D>)
    {
      std::vector<FaceNeighborType> validFaces = {Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor};
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
    if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
    {
      std::vector<FaceNeighborType> negYValidFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor,
                                                      Neighbors::k_PositiveZNeighbor};
      std::vector<FaceNeighborType> posYValidFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor,
                                                      Neighbors::k_PositiveZNeighbor};
      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
        {
          processFaceFunction(zIndex, 0, xIndex, negYValidFaces);
          processFaceFunction(zIndex, dims[1] - 1, xIndex, posYValidFaces);
        }
      }
    }
    if constexpr(std::is_same_v<ImageDimensionStateT, EmptyYImage2D>)
    {
      std::vector<FaceNeighborType> validFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveZNeighbor};
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
    if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
    {
      std::vector<FaceNeighborType> negXValidFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_PositiveXNeighbor, Neighbors::k_PositiveYNeighbor,
                                                      Neighbors::k_PositiveZNeighbor};
      std::vector<FaceNeighborType> posXValidFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_NegativeXNeighbor, Neighbors::k_PositiveYNeighbor,
                                                      Neighbors::k_PositiveZNeighbor};
      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
        {
          processFaceFunction(zIndex, yIndex, 0, negXValidFaces);
          processFaceFunction(zIndex, yIndex, dims[0] - 1, posXValidFaces);
        }
      }
    }
    if constexpr(std::is_same_v<ImageDimensionStateT, EmptyXImage2D>)
    {
      std::vector<FaceNeighborType> validFaces = {Neighbors::k_NegativeZNeighbor, Neighbors::k_NegativeYNeighbor, Neighbors::k_PositiveYNeighbor, Neighbors::k_PositiveZNeighbor};
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
} // namespace ImageDimensionalUtilities
} // namespace nx::core
