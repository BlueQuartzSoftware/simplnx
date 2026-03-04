#include "ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

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

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Z-slice buffering: maintain rolling window of 3 adjacent Z-slices for mask
  // to avoid random OOC chunk access during neighbor lookups.
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling window: slot 0 = z-1, slot 1 = z (current), slot 2 = z+1
  std::array<std::vector<bool>, 3> maskSlices;
  for(auto& ms : maskSlices)
  {
    ms.resize(sliceSize);
  }
  // maskCopy uses same rolling window structure for output
  std::array<std::vector<bool>, 3> maskCopySlices;
  for(auto& ms : maskCopySlices)
  {
    ms.resize(sliceSize);
  }

  auto readMaskSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    for(usize i = 0; i < sliceSize; i++)
    {
      maskSlices[slot][i] = mask[zOffset + i];
      maskCopySlices[slot][i] = maskSlices[slot][i];
    }
  };

  // Face neighbor ordering: 0=-Z, 1=-Y, 2=-X, 3=+X, 4=+Y, 5=+Z
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  for(int32_t iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {}", iteration));

    // Initialize rolling window: load z=0 into slot 1, z=1 into slot 2
    readMaskSlice(0, 1);
    if(dims[2] > 1)
    {
      readMaskSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance rolling window for z > 0
      if(zIdx > 0)
      {
        std::swap(maskSlices[0], maskSlices[1]);
        std::swap(maskSlices[1], maskSlices[2]);
        std::swap(maskCopySlices[0], maskCopySlices[1]);
        std::swap(maskCopySlices[1], maskCopySlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readMaskSlice(zIdx + 1, 2);
        }
      }

      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);

          if(!maskSlices[1][inSlice])
          {
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

            const std::array<usize, 6> neighborInSlice = {
                inSlice,                                          // -Z: same xy position in prev slice
                static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                inSlice                                           // +Z: same xy position in next slice
            };

            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              if(m_InputValues->Operation == detail::k_DilateIndex && maskSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]])
              {
                maskCopySlices[1][inSlice] = true;
              }
              if(m_InputValues->Operation == detail::k_ErodeIndex && maskSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]])
              {
                maskCopySlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] = false;
              }
            }
          }
        }
      }

      // Write back the completed z-1 slice (or current if last z)
      if(zIdx > 0)
      {
        const usize prevZOffset = static_cast<usize>(zIdx - 1) * sliceSize;
        for(usize i = 0; i < sliceSize; i++)
        {
          mask[prevZOffset + i] = maskCopySlices[0][i];
        }
      }
    }

    // Write back the last slice(s)
    if(dims[2] == 1)
    {
      for(usize i = 0; i < sliceSize; i++)
      {
        mask[i] = maskCopySlices[1][i];
      }
    }
    else
    {
      const usize lastZOffset = static_cast<usize>(dims[2] - 1) * sliceSize;
      for(usize i = 0; i < sliceSize; i++)
      {
        mask[lastZOffset + i] = maskCopySlices[1][i];
      }
    }
  }

  return {};
}
