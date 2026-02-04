#pragma once

#include "simplnx/Common/Types.hpp"

#include <array>

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
constexpr FaceNeighborType k_NegativeZNeighbor = 0;
constexpr FaceNeighborType k_NegativeYNeighbor = 1;
constexpr FaceNeighborType k_NegativeXNeighbor = 2;
constexpr FaceNeighborType k_PositiveXNeighbor = 3;
constexpr FaceNeighborType k_PositiveYNeighbor = 4;
constexpr FaceNeighborType k_PositiveZNeighbor = 5;
constexpr FaceNeighborType k_FaceNeighborCount = 6;

/**
 * @brief
 * @return
 */
inline std::array<FaceNeighborType, 6> initializeFaceNeighborInternalIdx()
{
  return {k_NegativeZNeighbor, k_NegativeYNeighbor, k_NegativeXNeighbor, k_PositiveXNeighbor, k_PositiveYNeighbor, k_PositiveZNeighbor};
}

/**
 * @brief Get the 6 face-connected neighbor offsets
 * @param dims Image geometry dimensions
 * @return Array of 6 neighbor offsets
 */
inline std::array<int64, 6> initializeFaceNeighborOffsets(const std::array<int64, 3>& dims)
{
  return {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};
}

/**
 * @brief Returns a boolean array of 6 elements that indicate if the given neighbor is within the bounds of the virtual volume
 * @param xIdx The x index (column) of the current voxel
 * @param yIdx The y index (row) of the current voxel
 * @param zIdx The z index (plane) of the current voxel
 * @param dims The dimensions of the virtual volume
 * @return
 */
inline std::array<bool, 6> computeValidFaceNeighbors(int64 xIdx, int64 yIdx, int64 zIdx, const std::array<int64, 3>& dims)
{
  return {zIdx > 0, yIdx > 0, xIdx > 0, xIdx < dims[0] - 1, yIdx < dims[1] - 1, zIdx < dims[2] - 1};
}

} // namespace nx::core
