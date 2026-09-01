/**
 * @file ErodeDilateMask.cpp
 * @brief Applies synchronous mask morphology with dual slice windows.
 */

#include "ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
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
const std::atomic_bool& ErodeDilateMask::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateMask::operator()()
{

  auto& mask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);
  const usize totalPoints = mask.getNumberOfTuples();

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Precompute face-neighbor index offsets and iteration order
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Three adjacent Z slices prevent random per-voxel store reads.
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // READ buffer: maskSlices holds the original mask state for this iteration.
  // Slot 0 = z-1, slot 1 = z (current), slot 2 = z+1.
  // Uses uint8 (0/1) because std::vector<bool> is bit-packed and does not
  // support pointer-based element access.
  std::array<std::vector<uint8>, 3> maskSlices;
  for(auto& ms : maskSlices)
  {
    ms.resize(sliceSize);
  }

  // WRITE buffer: maskCopySlices accumulates modifications for this iteration.
  // Starts as a copy of maskSlices and is modified during the scan.
  std::array<std::vector<uint8>, 3> maskCopySlices;
  for(auto& ms : maskCopySlices)
  {
    ms.resize(sliceSize);
  }

  // Temporary bool[] buffer for bulk I/O with the data store.
  // copyIntoBuffer/copyFromBuffer require contiguous bool arrays,
  // so we convert between bool and uint8 during reads and writes.
  auto boolBuf = std::make_unique<bool[]>(sliceSize);
  auto& maskStore = mask.getDataStoreRef();

  // Reads one Z-slice from the data store into both the read and write buffers.
  // The bool->uint8 conversion happens here.
  auto readMaskSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    maskStore.copyIntoBuffer(zOffset, nonstd::span<bool>(boolBuf.get(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      maskSlices[slot][i] = boolBuf[i] ? 1 : 0;
      maskCopySlices[slot][i] = maskSlices[slot][i];
    }
  };

  // Maps face-neighbor index to rolling-window slot:
  // -Z -> slot 0, -Y/-X/+X/+Y -> slot 1 (same Z), +Z -> slot 2
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // ---- Main iteration loop ----
  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {}", iteration));

    // Re-initialize rolling window from the (potentially modified) store.
    // z=0 -> slot 1 (current), z=1 -> slot 2 (next).
    readMaskSlice(0, 1);
    if(dims[2] > 1)
    {
      readMaskSlice(1, 2);
    }

    // ---- Z-slice scan loop ----
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance both read and write rolling windows for z > 0
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

      // ---- Inner XY scan ----
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);

          // Only process false (unmasked) voxels -- these are the boundary
          // candidates for both dilation and erosion.
          if(maskSlices[1][inSlice] == 0)
          {
            // Determine valid face neighbors and mask off user-disabled directions
            std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            if(!m_InputValues->XDirOn)
            {
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeXNeighbor] = false;
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveXNeighbor] = false;
            }
            if(!m_InputValues->YDirOn)
            {
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeYNeighbor] = false;
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveYNeighbor] = false;
            }
            if(!m_InputValues->ZDirOn)
            {
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeZNeighbor] = false;
              isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveZNeighbor] = false;
            }

            // Map each face neighbor to its in-slice offset within the
            // appropriate rolling-window slot.
            const std::array<usize, 6> neighborInSlice = {
                inSlice,                                         // -Z: same xy position in prev slice
                static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                inSlice                                          // +Z: same xy position in next slice
            };

            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              if(m_InputValues->Operation == detail::k_DilateIndex && maskSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] != 0)
              {
                // DILATION: This false voxel has a true neighbor, so it
                // should become true. Write into the copy buffer for the
                // current slice (slot 1).
                maskCopySlices[1][inSlice] = 1;
              }
              if(m_InputValues->Operation == detail::k_ErodeIndex && maskSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] != 0)
              {
                // EROSION: This false voxel has a true neighbor. Set the
                // neighbor to false in the copy buffer. The neighbor may
                // be in a different Z-slice (slot 0 or 2), which is why
                // we write to maskCopySlices[k_NeighborSlot[faceIndex]].
                maskCopySlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] = 0;
              }
            }
          }
        }
      }

      // ---- Write back the completed z-1 slice using bulk I/O ----
      // After processing slice z, the modifications for z-1 are finalized
      // (no future voxel at z+1 or beyond can affect z-1's write buffer).
      // Convert uint8 -> bool and write back via copyFromBuffer.
      if(zIdx > 0)
      {
        const usize prevZOffset = static_cast<usize>(zIdx - 1) * sliceSize;
        for(usize i = 0; i < sliceSize; i++)
        {
          boolBuf[i] = (maskCopySlices[0][i] != 0);
        }
        maskStore.copyFromBuffer(prevZOffset, nonstd::span<const bool>(boolBuf.get(), sliceSize));
      }
    }

    // ---- Flush the last (or only) Z-slice ----
    if(dims[2] == 1)
    {
      // Single-slice volume: the current slot is still in position 1
      for(usize i = 0; i < sliceSize; i++)
      {
        boolBuf[i] = (maskCopySlices[1][i] != 0);
      }
      maskStore.copyFromBuffer(0, nonstd::span<const bool>(boolBuf.get(), sliceSize));
    }
    else
    {
      // Multi-slice volume: after the loop, the last slice ended up in
      // slot 1 (due to swaps), which corresponds to dims[2]-1.
      const usize lastZOffset = static_cast<usize>(dims[2] - 1) * sliceSize;
      for(usize i = 0; i < sliceSize; i++)
      {
        boolBuf[i] = (maskCopySlices[1][i] != 0);
      }
      maskStore.copyFromBuffer(lastZOffset, nonstd::span<const bool>(boolBuf.get(), sliceSize));
    }
  }

  return {};
}
