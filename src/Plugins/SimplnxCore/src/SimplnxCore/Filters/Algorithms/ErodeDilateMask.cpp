#include "ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief Masks out face neighbors whose axis has been disabled via the X/Y/Z Direction parameters.
 * Indices follow the VoxelNeighbors<Image3D> ordering: [-Z,-Y,-X,+X,+Y,+Z].
 * @param isValidFaceNeighbor Per-voxel face-neighbor validity, already computed from geometry boundary.
 * @param xDir Whether the X direction is enabled.
 * @param yDir Whether the Y direction is enabled.
 * @param zDir Whether the Z direction is enabled.
 */
void adjustValidNeighbors(std::array<bool, VoxelNeighbors<Image3D>::k_FaceNeighborCount>& isValidFaceNeighbor, bool xDir, bool yDir, bool zDir)
{
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeZNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeZNeighbor] && zDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeYNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeYNeighbor] && yDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeXNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeXNeighbor] && xDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveXNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveXNeighbor] && xDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveYNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveYNeighbor] && yDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveZNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveZNeighbor] && zDir;
}
} // namespace

// -----------------------------------------------------------------------------
ErodeDilateMask::ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateMask::~ErodeDilateMask() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateMask::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateMask::operator()()
{

  auto& mask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);
  const size_t totalPoints = mask.getNumberOfTuples();

  std::vector<bool> maskCopy(totalPoints, false);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  for(int32_t iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {}", iteration));

    for(size_t j = 0; j < totalPoints; j++)
    {
      maskCopy[j] = mask[j];
    }
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      const int64 zStride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        const int64 yStride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = zStride + yStride + xIdx;

          if(!mask[voxelIndex])
          {
            // Loop over the 6 face neighbors of the voxel
            std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            adjustValidNeighbors(isValidFaceNeighbor, m_InputValues->XDirOn, m_InputValues->YDirOn, m_InputValues->ZDirOn);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              const int64 neighpoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

              if(m_InputValues->Operation == detail::k_DilateIndex && mask[neighpoint])
              {
                maskCopy[voxelIndex] = true;
              }
              if(m_InputValues->Operation == detail::k_ErodeIndex && mask[neighpoint])
              {
                maskCopy[neighpoint] = false;
              }
            }
          }
        }
      }
    }
    for(size_t j = 0; j < totalPoints; j++)
    {
      mask[j] = maskCopy[j];
    }
  }

  return {};
}
