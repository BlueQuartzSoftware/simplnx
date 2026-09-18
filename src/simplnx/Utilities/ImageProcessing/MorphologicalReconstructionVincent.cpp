#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionWavefront.hpp"

#include <new>

namespace nx::core::ImageProcessing
{
template <class T>
Result<> ReconstructVincent<T>::operator()()
{
  return (m_Op == ReconstructOp::Dilation) ? runImpl<true>() : runImpl<false>();
}

template <class T>
template <bool Dilation>
Result<> ReconstructVincent<T>::runImpl()
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const int64 nX = static_cast<int64>(m_Dims[0]);
  const int64 nY = static_cast<int64>(m_Dims[1]);
  const int64 nZ = static_cast<int64>(m_Dims[2]);
  const usize vol = static_cast<usize>(nX * nY * nZ);
  if(vol == 0)
  {
    return {};
  }

  auto work = std::make_unique<T[]>(vol);
  auto mask = std::make_unique<T[]>(vol);
  if(m_MarkerOptions.source == ReconstructionMarkerSource::Provided)
  {
    if(Result<> r = m_Marker.copyIntoBuffer(0, nonstd::span<T>(work.get(), vol)); r.invalid())
    {
      return r;
    }
  }
  if(Result<> r = m_Mask.copyIntoBuffer(0, nonstd::span<T>(mask.get(), vol)); r.invalid())
  {
    return r;
  }
  if(m_MarkerOptions.source != ReconstructionMarkerSource::Provided)
  {
    std::copy_n(mask.get(), vol, work.get());
    detail::ApplyReconstructionMarkerOptions<T>(nonstd::span<T>(work.get(), vol), 0, m_Dims, m_MarkerOptions);
  }

  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
  auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  // flat() is affine, so an in-bounds neighbor's index is simply p + (a constant delta per offset). Precompute
  // the delta once per offset and add it in the hot loops, instead of recomputing the multiply-based flat() for
  // every neighbor of every voxel. Offset and delta travel together so the bounds check (on coordinates) and the
  // index computation cannot drift apart. Byte-for-byte identical to flat(); it only removes the per-neighbor
  // multiplies.
  struct NeighborDelta
  {
    detail::ReconOffset off;
    int64 delta;
  };
  const int64 nXY = nX * nY;
  auto withDeltas = [nXY, nX](const std::vector<detail::ReconOffset>& offs) {
    std::vector<NeighborDelta> nd;
    nd.reserve(offs.size());
    for(const detail::ReconOffset& o : offs)
    {
      nd.push_back(NeighborDelta{o, static_cast<int64>(o.dz) * nXY + static_cast<int64>(o.dy) * nX + static_cast<int64>(o.dx)});
    }
    return nd;
  };
  const std::vector<NeighborDelta> prevN = withDeltas(offsets.previous);
  const std::vector<NeighborDelta> laterN = withDeltas(offsets.later);
  const std::vector<NeighborDelta> allN = withDeltas(offsets.all);

  // The block-wavefront schedule below tiles each plane into blockEdge x blockEdge blocks and groups them into
  // waves w = 2*blockRow + blockCol, visited in ascending order for a forward sweep and descending order for
  // reverse (see RunPlaneWavefront's documentation for the full schedule proof). That schedule is sound for FACE
  // connectivity's in-plane predecessor set {N, W} / {S, E}, which has no diagonal offset. It is NOT sound for
  // FULL connectivity's one-pass Phase 1 / Phase 2 raster sweeps: a diagonal neighbor whose row step stays inside
  // the current row band lands in the horizontally ADJACENT block rather than a vertically prior one -- forward's
  // NE reads block (blockRow, blockCol+1) = wave w+1, and reverse's SW reads block (blockRow, blockCol-1) = wave
  // w-1 under descending order -- a genuine horizontal block-dependency cycle no rectangular row-major schedule
  // can order. Phase 1, Phase 2, and sweepHalf therefore keep the exact serial raster for full connectivity and
  // only take the wavefront-parallel path below when the connectivity is face-only. Building the schedule is
  // O(block count), not O(volume), so computing it unconditionally is cheap even for the full-connectivity case
  // that never uses it.
  // Border-derived markers (MaskWithMinimumInterior / MaskWithMaximumInterior) seed nearly the whole volume, so
  // their reconstructions overflow the bounded frontier and spend their time in finishWithResidentSweeps'
  // fold-to-convergence pairs. Across that many sweep pairs, the per-plane per-wave parallel dispatch overhead of
  // the block wavefront outweighs its in-plane parallel gain, so those markers keep the fully serial rasters; the
  // provided and height-derived markers converge in few passes and take the wavefront path.
  const bool borderDerivedMarker = m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMinimumInterior || m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMaximumInterior;
  const bool useWavefront = !m_FullyConnected && !borderDerivedMarker;
  const detail::ReconstructionPlaneWavefrontSchedule planeWavefrontSchedule = detail::BuildReconstructionPlaneWavefrontSchedule(m_Dims[0], m_Dims[1]);

  auto sweepHalf = [&](bool forward, const std::vector<NeighborDelta>& neighbors, bool& changed) {
    if(useWavefront)
    {
      for(usize planeIndex = 0; planeIndex < static_cast<usize>(nZ); ++planeIndex)
      {
        if(m_ShouldCancel)
        {
          return false;
        }
        const int64 z = forward ? static_cast<int64>(planeIndex) : nZ - 1 - static_cast<int64>(planeIndex);
        // Cross-plane deltas in `neighbors` read plane z's already-finished predecessor (z-1 for forward, z+1 for
        // reverse), which the previous outer-loop iteration fully completed and synchronized before this
        // iteration begins -- an ordinary read of already-settled memory, not a race.
        auto processBlock = [&](const detail::ReconstructionPlaneBlock& block) -> bool {
          bool blockChanged = false;
          const usize rowSpan = block.rowEnd - block.rowBegin;
          const usize colSpan = block.colEnd - block.colBegin;
          for(usize localRow = 0; localRow < rowSpan; ++localRow)
          {
            const usize rowIndex = forward ? block.rowBegin + localRow : block.rowEnd - 1 - localRow;
            const int64 y = static_cast<int64>(rowIndex);
            for(usize localCol = 0; localCol < colSpan; ++localCol)
            {
              const usize columnIndex = forward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
              const int64 x = static_cast<int64>(columnIndex);
              const usize position = flat(x, y, z);
              T value = work[position];
              for(const NeighborDelta& neighbor : neighbors)
              {
                if(inBounds(x + neighbor.off.dx, y + neighbor.off.dy, z + neighbor.off.dz))
                {
                  value = Tr::fold(value, work[static_cast<usize>(static_cast<int64>(position) + neighbor.delta)]);
                }
              }
              const T newValue = Tr::clampToMask(value, mask[position]);
              if(newValue != work[position])
              {
                work[position] = newValue;
                blockChanged = true;
              }
            }
          }
          return blockChanged;
        };
        if(detail::RunPlaneWavefront(planeWavefrontSchedule, forward, processBlock))
        {
          changed = true;
        }
      }
      return true;
    }
    for(usize planeIndex = 0; planeIndex < static_cast<usize>(nZ); ++planeIndex)
    {
      if(m_ShouldCancel)
      {
        return false;
      }
      const int64 z = forward ? static_cast<int64>(planeIndex) : nZ - 1 - static_cast<int64>(planeIndex);
      for(usize rowIndex = 0; rowIndex < static_cast<usize>(nY); ++rowIndex)
      {
        const int64 y = forward ? static_cast<int64>(rowIndex) : nY - 1 - static_cast<int64>(rowIndex);
        for(usize columnIndex = 0; columnIndex < static_cast<usize>(nX); ++columnIndex)
        {
          const int64 x = forward ? static_cast<int64>(columnIndex) : nX - 1 - static_cast<int64>(columnIndex);
          const usize position = flat(x, y, z);
          T value = work[position];
          for(const NeighborDelta& neighbor : neighbors)
          {
            if(inBounds(x + neighbor.off.dx, y + neighbor.off.dy, z + neighbor.off.dz))
            {
              value = Tr::fold(value, work[static_cast<usize>(static_cast<int64>(position) + neighbor.delta)]);
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
    return true;
  };
  auto finishWithResidentSweeps = [&]() -> Result<> {
    bool changed = false;
    do
    {
      changed = false;
      if(!sweepHalf(true, prevN, changed) || !sweepHalf(false, laterN, changed))
      {
        return {};
      }
    } while(changed);
    return m_Out.copyFromBuffer(0, nonstd::span<const T>(work.get(), vol));
  };

  usize frontierCapacity = m_FrontierCapacity;
  if(frontierCapacity == std::numeric_limits<usize>::max())
  {
    auto frontierBytesResult = detail::CalculateReconstructionFrontierWorkingMemoryBytes(m_Dims);
    if(frontierBytesResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(frontierBytesResult));
    }
    frontierCapacity = frontierBytesResult.value() / sizeof(uint64);
  }
  std::unique_ptr<detail::BoundedReconstructionQueue<uint64>> fifo;
  try
  {
    if(frontierCapacity > 0)
    {
      fifo = std::make_unique<detail::BoundedReconstructionQueue<uint64>>(std::min(vol, frontierCapacity));
    }
  } catch(const std::bad_alloc&)
  {
    return finishWithResidentSweeps();
  }
  if(fifo == nullptr)
  {
    return finishWithResidentSweeps();
  }

  // --- Phase 1: forward raster scan over PREVIOUS neighbors (reads already-updated work). ---
  if(useWavefront)
  {
    for(int64 z = 0; z < nZ; ++z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      // Cross-plane (dz<0) reads in prevN hit plane z-1, which the previous outer-loop iteration fully finished
      // and synchronized (RunPlaneWavefront barriers every wave before the next starts) before this iteration
      // begins -- an ordinary read of already-settled memory, not a race.
      auto processBlock = [&](const detail::ReconstructionPlaneBlock& block) -> bool {
        bool blockChanged = false;
        for(usize row = block.rowBegin; row < block.rowEnd; ++row)
        {
          const int64 y = static_cast<int64>(row);
          for(usize col = block.colBegin; col < block.colEnd; ++col)
          {
            const int64 x = static_cast<int64>(col);
            const usize p = flat(x, y, z);
            T v = work[p];
            for(const NeighborDelta& nd : prevN)
            {
              if(inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
              {
                v = Tr::fold(v, work[static_cast<usize>(static_cast<int64>(p) + nd.delta)]);
              }
            }
            // The wavefront path writes conditionally, tracking blockChanged for RunPlaneWavefront's per-block
            // changed-flag contract, unlike the unconditional assignment in the serial path below; both leave
            // work[p] holding the identical final value.
            const T newValue = Tr::clampToMask(v, mask[p]);
            if(newValue != work[p])
            {
              work[p] = newValue;
              blockChanged = true;
            }
          }
        }
        return blockChanged;
      };
      // Phase 1 never tracked a changed flag in the original serial code, so the aggregate return is discarded
      // here too, matching the original.
      static_cast<void>(detail::RunPlaneWavefront(planeWavefrontSchedule, /*forward=*/true, processBlock));
    }
  }
  else
  {
    for(int64 z = 0; z < nZ; ++z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      for(int64 y = 0; y < nY; ++y)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          const usize p = flat(x, y, z);
          T v = work[p];
          for(const NeighborDelta& nd : prevN)
          {
            if(inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
            {
              v = Tr::fold(v, work[static_cast<usize>(static_cast<int64>(p) + nd.delta)]);
            }
          }
          work[p] = Tr::clampToMask(v, mask[p]);
        }
      }
    }
  }

  // --- Phase 2: reverse anti-raster scan over LATER neighbors + FIFO seeding. ---
  if(useWavefront)
  {
    bool overflowed = false;
    for(int64 z = nZ - 1; z >= 0 && !overflowed; --z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      // Cross-plane (dz>0) reads in laterN hit plane z+1, which the previous (descending) outer-loop iteration
      // fully finished and synchronized before this iteration begins -- an ordinary read of already-settled
      // memory, not a race.
      auto processBlock = [&](const detail::ReconstructionPlaneBlock& block, std::vector<uint64>& seeds) {
        const usize rowSpan = block.rowEnd - block.rowBegin;
        const usize colSpan = block.colEnd - block.colBegin;
        for(usize localRow = 0; localRow < rowSpan; ++localRow)
        {
          const usize rowIndex = block.rowEnd - 1 - localRow;
          const int64 y = static_cast<int64>(rowIndex);
          for(usize localCol = 0; localCol < colSpan; ++localCol)
          {
            const usize colIndex = block.colEnd - 1 - localCol;
            const int64 x = static_cast<int64>(colIndex);
            const usize p = flat(x, y, z);
            T v = work[p];
            for(const NeighborDelta& nd : laterN)
            {
              if(inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
              {
                v = Tr::fold(v, work[static_cast<usize>(static_cast<int64>(p) + nd.delta)]);
              }
            }
            v = Tr::clampToMask(v, mask[p]);
            work[p] = v;
            for(const NeighborDelta& nd : laterN)
            {
              if(!inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
              {
                continue;
              }
              const usize q = static_cast<usize>(static_cast<int64>(p) + nd.delta);
              if(Tr::compare(v, work[q]) && Tr::compare(mask[q], work[q]))
              {
                seeds.push_back(static_cast<uint64>(p));
                break;
              }
            }
          }
        }
      };
      const bool allSeedsAccepted = detail::RunReversePlaneWavefrontSeeding(planeWavefrontSchedule, processBlock, [&fifo](uint64 seed) { return fifo->tryPush(seed); });
      if(!allSeedsAccepted)
      {
        overflowed = true;
      }
    }
    if(overflowed)
    {
      return finishWithResidentSweeps();
    }
  }
  else
  {
    for(int64 z = nZ - 1; z >= 0; --z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      for(int64 y = nY - 1; y >= 0; --y)
      {
        for(int64 x = nX - 1; x >= 0; --x)
        {
          const usize p = flat(x, y, z);
          T v = work[p];
          for(const NeighborDelta& nd : laterN)
          {
            if(inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
            {
              v = Tr::fold(v, work[static_cast<usize>(static_cast<int64>(p) + nd.delta)]);
            }
          }
          v = Tr::clampToMask(v, mask[p]);
          work[p] = v;
          for(const NeighborDelta& nd : laterN)
          {
            if(!inBounds(x + nd.off.dx, y + nd.off.dy, z + nd.off.dz))
            {
              continue;
            }
            const usize q = static_cast<usize>(static_cast<int64>(p) + nd.delta);
            if(Tr::compare(v, work[q]) && Tr::compare(mask[q], work[q]))
            {
              if(!fifo->tryPush(static_cast<uint64>(p)))
              {
                return finishWithResidentSweeps();
              }
              break;
            }
          }
        }
      }
    }
  }

  // --- Phase 3: FIFO flood over the FULL neighborhood (random access). ---
  const usize sliceValues = static_cast<usize>(nX * nY);
  while(!fifo->empty())
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize p = fifo->front();
    fifo->pop();
    const int64 px = static_cast<int64>(p % static_cast<usize>(nX));
    const int64 py = static_cast<int64>((p / static_cast<usize>(nX)) % static_cast<usize>(nY));
    const int64 pz = static_cast<int64>(p / sliceValues);
    const T v = work[p];
    for(const NeighborDelta& nd : allN)
    {
      if(!inBounds(px + nd.off.dx, py + nd.off.dy, pz + nd.off.dz))
      {
        continue;
      }
      const usize q = static_cast<usize>(static_cast<int64>(p) + nd.delta);
      if(Tr::compare(v, work[q]) && work[q] != mask[q])
      {
        work[q] = Tr::compare(mask[q], v) ? v : mask[q];
        if(!fifo->tryPush(static_cast<uint64>(q)))
        {
          return finishWithResidentSweeps();
        }
      }
    }
  }

  return m_Out.copyFromBuffer(0, nonstd::span<const T>(work.get(), vol));
}

template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int8>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int16>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int32>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int64>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<float32>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<float64>;
} // namespace nx::core::ImageProcessing
