#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"

#include <new>

namespace nx::core::ImageProcessing
{
template <class T, bool UseTemporaryWork>
template <bool Dilation>
bool ReconstructSweep<T, UseTemporaryWork>::runResidentFrontier(T* work, const T* mask, usize dimX, usize dimY, usize dimZ, usize sliceValues, detail::BoundedReconstructionQueue<uint64>& frontier)
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  struct NeighborDelta
  {
    detail::ReconOffset offset;
    int64 delta;
  };
  const int64 nXY = nX * nY;
  auto withDeltas = [nXY, nX](const std::vector<detail::ReconOffset>& offsets) {
    std::vector<NeighborDelta> neighbors;
    neighbors.reserve(offsets.size());
    for(const detail::ReconOffset& offset : offsets)
    {
      neighbors.push_back(NeighborDelta{offset, static_cast<int64>(offset.dz) * nXY + static_cast<int64>(offset.dy) * nX + static_cast<int64>(offset.dx)});
    }
    return neighbors;
  };
  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
  const std::vector<NeighborDelta> previousNeighbors = withDeltas(offsets.previous);
  const std::vector<NeighborDelta> laterNeighbors = withDeltas(offsets.later);
  const std::vector<NeighborDelta> allNeighbors = withDeltas(offsets.all);

  for(int64 z = 0; z < nZ; ++z)
  {
    if(m_ShouldCancel)
    {
      return false;
    }
    for(int64 y = 0; y < nY; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        const usize position = flat(x, y, z);
        T value = work[position];
        for(const NeighborDelta& neighbor : previousNeighbors)
        {
          if(inBounds(x + neighbor.offset.dx, y + neighbor.offset.dy, z + neighbor.offset.dz))
          {
            value = Tr::fold(value, work[static_cast<usize>(static_cast<int64>(position) + neighbor.delta)]);
          }
        }
        work[position] = Tr::clampToMask(value, mask[position]);
      }
    }
  }

  for(int64 z = nZ - 1; z >= 0; --z)
  {
    if(m_ShouldCancel)
    {
      return false;
    }
    for(int64 y = nY - 1; y >= 0; --y)
    {
      for(int64 x = nX - 1; x >= 0; --x)
      {
        const usize position = flat(x, y, z);
        T value = work[position];
        for(const NeighborDelta& neighbor : laterNeighbors)
        {
          if(inBounds(x + neighbor.offset.dx, y + neighbor.offset.dy, z + neighbor.offset.dz))
          {
            value = Tr::fold(value, work[static_cast<usize>(static_cast<int64>(position) + neighbor.delta)]);
          }
        }
        value = Tr::clampToMask(value, mask[position]);
        work[position] = value;
        for(const NeighborDelta& neighbor : laterNeighbors)
        {
          if(!inBounds(x + neighbor.offset.dx, y + neighbor.offset.dy, z + neighbor.offset.dz))
          {
            continue;
          }
          const usize neighborPosition = static_cast<usize>(static_cast<int64>(position) + neighbor.delta);
          if(Tr::compare(value, work[neighborPosition]) && Tr::compare(mask[neighborPosition], work[neighborPosition]))
          {
            if(!frontier.tryPush(static_cast<uint64>(position)))
            {
              return false;
            }
            break;
          }
        }
      }
    }
  }

  while(!frontier.empty())
  {
    if(m_ShouldCancel)
    {
      return false;
    }
    const usize position = frontier.front();
    frontier.pop();
    const int64 x = static_cast<int64>(position % dimX);
    const int64 y = static_cast<int64>((position / dimX) % dimY);
    const int64 z = static_cast<int64>(position / sliceValues);
    const T value = work[position];
    for(const NeighborDelta& neighbor : allNeighbors)
    {
      if(!inBounds(x + neighbor.offset.dx, y + neighbor.offset.dy, z + neighbor.offset.dz))
      {
        continue;
      }
      const usize neighborPosition = static_cast<usize>(static_cast<int64>(position) + neighbor.delta);
      if(Tr::compare(value, work[neighborPosition]) && work[neighborPosition] != mask[neighborPosition])
      {
        work[neighborPosition] = Tr::compare(mask[neighborPosition], value) ? value : mask[neighborPosition];
        if(!frontier.tryPush(static_cast<uint64>(neighborPosition)))
        {
          return false;
        }
      }
    }
  }
  return true;
}

template <class T, bool UseTemporaryWork>
template <bool Dilation>
Result<> ReconstructSweep<T, UseTemporaryWork>::runResident3D(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol)
{
  using Tr = detail::ReconTraits<T, Dilation>;

  // Capacity comes from the active shared-budget reservation. The branch is used only when that fixed capacity
  // covers the complete logical image. Larger images use the external bounded fallback above.
  auto work = std::make_unique<T[]>(m_MaxSlabValues);
  auto mask = std::make_unique<T[]>(m_MaxSlabValues);
  if(m_MarkerOptions.source == ReconstructionMarkerSource::Provided)
  {
    if(Result<> result = m_Marker.copyIntoBuffer(0, nonstd::span<T>(work.get(), vol)); result.invalid())
    {
      return result;
    }
  }
  if(Result<> result = m_Mask.copyIntoBuffer(0, nonstd::span<T>(mask.get(), vol)); result.invalid())
  {
    return result;
  }
  if(m_MarkerOptions.source != ReconstructionMarkerSource::Provided)
  {
    std::copy_n(mask.get(), vol, work.get());
    detail::ApplyReconstructionMarkerOptions<T>(nonstd::span<T>(work.get(), vol), 0, m_Dims, m_MarkerOptions);
  }

  if(m_ResidentFrontierValues > 0)
  {
    try
    {
      detail::BoundedReconstructionQueue<uint64> frontier(std::min(vol, m_ResidentFrontierValues));
      if(runResidentFrontier<Dilation>(work.get(), mask.get(), dimX, dimY, dimZ, sliceValues, frontier))
      {
        return m_Out.copyFromBuffer(0, nonstd::span<const T>(work.get(), vol));
      }
    } catch(const std::bad_alloc&)
    {
      // The queue is optional. The resident sweep below uses the already-reserved work and mask arrays.
    }
    if(m_ShouldCancel)
    {
      return {};
    }
  }

  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  auto flat = [dimX, dimY](usize x, usize y, usize z) { return z * dimY * dimX + y * dimX + x; };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  auto sweep = [&](bool forward, bool& changed) {
    const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
    for(usize planeIndex = 0; planeIndex < dimZ; ++planeIndex)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize z = forward ? planeIndex : dimZ - 1 - planeIndex;
      for(usize rowIndex = 0; rowIndex < dimY; ++rowIndex)
      {
        const usize y = forward ? rowIndex : dimY - 1 - rowIndex;
        for(usize columnIndex = 0; columnIndex < dimX; ++columnIndex)
        {
          const usize x = forward ? columnIndex : dimX - 1 - columnIndex;
          const usize position = flat(x, y, z);
          T value = work[position];
          if(!m_FullyConnected)
          {
            if(forward)
            {
              if(z > 0)
              {
                value = Tr::fold(value, work[position - sliceValues]);
              }
              if(y > 0)
              {
                value = Tr::fold(value, work[position - dimX]);
              }
              if(x > 0)
              {
                value = Tr::fold(value, work[position - 1]);
              }
            }
            else
            {
              if(x + 1 < dimX)
              {
                value = Tr::fold(value, work[position + 1]);
              }
              if(y + 1 < dimY)
              {
                value = Tr::fold(value, work[position + dimX]);
              }
              if(z + 1 < dimZ)
              {
                value = Tr::fold(value, work[position + sliceValues]);
              }
            }
          }
          else
          {
            for(const detail::ReconOffset& offset : half)
            {
              const int64 neighborX = static_cast<int64>(x) + offset.dx;
              const int64 neighborY = static_cast<int64>(y) + offset.dy;
              const int64 neighborZ = static_cast<int64>(z) + offset.dz;
              if(inBounds(neighborX, neighborY, neighborZ))
              {
                value = Tr::fold(value, work[flat(static_cast<usize>(neighborX), static_cast<usize>(neighborY), static_cast<usize>(neighborZ))]);
              }
            }
          }
          const T newValue = Tr::clampToMask(value, mask[position]);
          if(newValue != work[position])
          {
            work[position] = newValue;
            changed = true;
          }
        }
      }
    }
  };

  for(usize sweepPair = 0; sweepPair < m_MaxSweepPairs; ++sweepPair)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    bool changed = false;
    sweep(true, changed);
    if(m_ShouldCancel)
    {
      return {};
    }
    sweep(false, changed);
    if(!changed)
    {
      break;
    }
  }
  return m_Out.copyFromBuffer(0, nonstd::span<const T>(work.get(), vol));
}

template <class T, bool UseTemporaryWork>
template <bool Dilation>
Result<> ReconstructSweep<T, UseTemporaryWork>::runDirect(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol)
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);

  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);

  // At least one plane must fit so exceptionally wide planes remain supported; in that case a single working
  // plane can necessarily exceed the nominal byte target. Every other slab stays at or below the value budget.
  const usize slabPlanes = std::min(dimZ, std::max<usize>(1, m_MaxSlabValues / sliceValues));
  const usize slabCapacity = slabPlanes * sliceValues;
  auto workSlab = std::make_unique<T[]>(slabCapacity);
  auto maskSlab = std::make_unique<T[]>(slabCapacity);

  // Step 1: initialize the working store (== m_Out) to the marker with the same bounded slab buffer.
  for(usize start = 0; start < vol; start += slabCapacity)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(slabCapacity, vol - start);
    if(Result<> r = loadMarker(start, nonstd::span<T>(workSlab.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = m_Out.copyFromBuffer(start, nonstd::span<const T>(workSlab.get(), count)); r.invalid())
    {
      return r;
    }
  }

  // One boundary plane carries the last processed plane between slabs.
  auto adjacent = std::make_unique<T[]>(sliceValues);

  auto flat2d = [dimX](int64 x, int64 y) { return static_cast<usize>(y * static_cast<int64>(dimX) + x); };
  auto inBoundsXY = [nX, nY](int64 x, int64 y) { return x >= 0 && x < nX && y >= 0 && y < nY; };

  // A single directional sweep, in place over m_Out. Forward visits slabs and voxels ascending and uses PREVIOUS
  // neighbors; reverse visits them descending and uses LATER neighbors. Store I/O is serial and contiguous. All
  // updates remain sequential because later voxels read directly from the already-updated work slab.
  auto sweep = [&](bool forward, bool& changed) -> Result<> {
    const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
    bool haveAdjacent = false;
    for(usize slabIndex = 0; slabIndex < dimZ; slabIndex += slabPlanes)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize remainingPlanes = dimZ - slabIndex;
      const usize currentSlabPlanes = std::min(slabPlanes, remainingPlanes);
      const usize zBegin = forward ? slabIndex : (dimZ - slabIndex - currentSlabPlanes);
      const usize currentSlabValues = currentSlabPlanes * sliceValues;
      const usize slabStart = zBegin * sliceValues;
      if(Result<> r = m_Out.copyIntoBuffer(slabStart, nonstd::span<T>(workSlab.get(), currentSlabValues)); r.invalid())
      {
        return r;
      }
      if(Result<> r = m_Mask.copyIntoBuffer(slabStart, nonstd::span<T>(maskSlab.get(), currentSlabValues)); r.invalid())
      {
        return r;
      }
      bool slabChanged = false;
      for(usize planeIndex = 0; planeIndex < currentSlabPlanes; ++planeIndex)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize localZ = forward ? planeIndex : (currentSlabPlanes - 1 - planeIndex);
        const usize planeOffset = localZ * sliceValues;
        for(usize yi = 0; yi < dimY; ++yi)
        {
          const int64 y = forward ? static_cast<int64>(yi) : static_cast<int64>(dimY - 1 - yi);
          for(usize xi = 0; xi < dimX; ++xi)
          {
            const int64 x = forward ? static_cast<int64>(xi) : static_cast<int64>(dimX - 1 - xi);
            const usize p2d = flat2d(x, y);
            const usize p = planeOffset + p2d;
            T v = workSlab[p];
            if(!m_FullyConnected)
            {
              // Preserve MakeReconstructionOffsets' exact face-neighbor order while avoiding its generic offset,
              // bounds, and dz branches in the dominant 6-connected case. Forward order is Z-, Y-, X-; reverse
              // order is X+, Y+, Z+.
              if(forward)
              {
                if(localZ > 0)
                {
                  v = Tr::fold(v, workSlab[p - sliceValues]);
                }
                else if(haveAdjacent)
                {
                  v = Tr::fold(v, adjacent[p2d]);
                }
                if(y > 0)
                {
                  v = Tr::fold(v, workSlab[p - dimX]);
                }
                if(x > 0)
                {
                  v = Tr::fold(v, workSlab[p - 1]);
                }
              }
              else
              {
                if(x + 1 < nX)
                {
                  v = Tr::fold(v, workSlab[p + 1]);
                }
                if(y + 1 < nY)
                {
                  v = Tr::fold(v, workSlab[p + dimX]);
                }
                if(localZ + 1 < currentSlabPlanes)
                {
                  v = Tr::fold(v, workSlab[p + sliceValues]);
                }
                else if(haveAdjacent)
                {
                  v = Tr::fold(v, adjacent[p2d]);
                }
              }
            }
            else
            {
              for(const detail::ReconOffset& o : half)
              {
                const int64 nx = x + o.dx;
                const int64 ny = y + o.dy;
                if(!inBoundsXY(nx, ny))
                {
                  continue;
                }
                const usize neighbor2d = flat2d(nx, ny);
                if(o.dz == 0)
                {
                  v = Tr::fold(v, workSlab[planeOffset + neighbor2d]);
                }
                else
                {
                  const bool neighborInsideSlab = forward ? (localZ > 0) : (localZ + 1 < currentSlabPlanes);
                  if(neighborInsideSlab)
                  {
                    const usize neighborPlaneOffset = forward ? (planeOffset - sliceValues) : (planeOffset + sliceValues);
                    v = Tr::fold(v, workSlab[neighborPlaneOffset + neighbor2d]);
                  }
                  else if(haveAdjacent)
                  {
                    v = Tr::fold(v, adjacent[neighbor2d]);
                  }
                }
              }
            }
            const T newVal = Tr::clampToMask(v, maskSlab[p]);
            if(newVal != workSlab[p])
            {
              workSlab[p] = newVal;
              changed = true;
              slabChanged = true;
            }
          }
        }
      }
      // Capture a boundary plane only when another slab follows. Forward carries the slab's last plane; reverse
      // carries its first plane. The final slab has no downstream consumer for this copy.
      if(slabIndex + currentSlabPlanes < dimZ)
      {
        const usize boundaryOffset = forward ? ((currentSlabPlanes - 1) * sliceValues) : 0;
        std::copy_n(workSlab.get() + boundaryOffset, sliceValues, adjacent.get());
        haveAdjacent = true;
      }
      if(slabChanged)
      {
        if(Result<> r = m_Out.copyFromBuffer(slabStart, nonstd::span<const T>(workSlab.get(), currentSlabValues)); r.invalid())
        {
          return r;
        }
      }
    }
    return {};
  };

  for(usize sweepPair = 0; sweepPair < m_MaxSweepPairs; ++sweepPair)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    bool changed = false;
    if(Result<> r = sweep(true, changed); r.invalid())
    {
      return r;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    if(Result<> r = sweep(false, changed); r.invalid())
    {
      return r;
    }
    if(!changed)
    {
      break; // reached the fixpoint
    }
  }
  return {};
}

// The three members below are ODR-used only from ReconstructSweep::runImpl (defined in
// MorphologicalReconstructionSweepStreamed.cpp): runDirect from ReconstructSweep<T, false>::runImpl, and
// runResident3D + runResidentFrontier from ReconstructSweep<T, true>::runImpl. Both call sites compile into
// libsimplnx itself, so these member-level explicit instantiations carry no export macro -- the linker resolves
// them within the same shared library, and no other library calls these members directly.
#define SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(T)                                                                                                                                        \
  template Result<> ReconstructSweep<T, false>::runDirect<true>(usize, usize, usize, usize, usize);                                                                                                    \
  template Result<> ReconstructSweep<T, false>::runDirect<false>(usize, usize, usize, usize, usize);                                                                                                   \
  template Result<> ReconstructSweep<T, true>::runResident3D<true>(usize, usize, usize, usize, usize);                                                                                                 \
  template Result<> ReconstructSweep<T, true>::runResident3D<false>(usize, usize, usize, usize, usize);                                                                                                \
  template bool ReconstructSweep<T, true>::runResidentFrontier<true>(T*, const T*, usize, usize, usize, usize, detail::BoundedReconstructionQueue<uint64>&);                                           \
  template bool ReconstructSweep<T, true>::runResidentFrontier<false>(T*, const T*, usize, usize, usize, usize, detail::BoundedReconstructionQueue<uint64>&)

SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(int8);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(uint8);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(int16);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(uint16);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(int32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(uint32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(int64);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(uint64);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(float32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS(float64);

#undef SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_SERIAL_MEMBERS
} // namespace nx::core::ImageProcessing
