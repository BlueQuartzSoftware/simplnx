#include "ComputeBoundaryCellsDirect.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

ComputeBoundaryCellsDirect::ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       const ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeBoundaryCellsDirect::~ComputeBoundaryCellsDirect() noexcept = default;

Result<> ComputeBoundaryCellsDirect::operator()()
{
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // These operator[] calls can reach a disk-backed output because dispatch does
  // not include BoundaryCells.
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  // Precompute flat offsets for the six face-neighbor directions.
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  int32 feature = 0;
  int8 onSurf = 0;
  int64 neighborPoint = 0;

  // The lower bound excludes feature zero only when requested.
  int ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  int64 kStride = 0;
  int64 jStride = 0;

  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
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
          // Count volume faces only for axes with more than two cells.
          // Feature zero receives no volume-boundary contribution.
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

          const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            // Count a different neighbor that passes the feature-zero policy.
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
  return {};
}
