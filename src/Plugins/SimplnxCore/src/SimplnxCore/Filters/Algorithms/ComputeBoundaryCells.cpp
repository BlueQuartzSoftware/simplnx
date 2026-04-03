#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundaryCells::ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryCells::~ComputeBoundaryCells() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeBoundaryCells::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeBoundaryCells::operator()()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
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

  int32 feature = 0;
  int8 onSurf = 0;
  int64 neighborPoint = 0;

  int ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(dims[2]);
  progressHelper.setProgressMessageTemplate("Computing Boundary Cells: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  int64 kStride = 0;
  int64 jStride = 0;

  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    progressMessenger.sendProgressMessage(1);
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

          // Loop over the 6 face neighbors of the voxel
          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

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
