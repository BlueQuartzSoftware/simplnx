#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionWavefront.hpp"

#include <new>

namespace nx::core::ImageProcessing
{
template <class T, bool UseTemporaryWork>
template <bool Dilation>
Result<> ReconstructSweep<T, UseTemporaryWork>::runImpl()
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const usize dimX = m_Dims[0];
  const usize dimY = m_Dims[1];
  const usize dimZ = m_Dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return {};
  }
  if(dimX > std::numeric_limits<usize>::max() / dimY)
  {
    return MakeErrorResult(-8640, fmt::format("Morphological reconstruction dimensions overflow while computing the XY plane size: {} x {} x {}.", dimX, dimY, dimZ));
  }
  const usize sliceValues = dimX * dimY;
  if(sliceValues > std::numeric_limits<usize>::max() / dimZ)
  {
    return MakeErrorResult(-8641, fmt::format("Morphological reconstruction dimensions overflow while computing the XYZ volume size: {} x {} x {}.", dimX, dimY, dimZ));
  }
  const usize vol = sliceValues * dimZ;
  if(m_Marker.getSize() != vol || m_Mask.getSize() != vol || m_Out.getSize() != vol)
  {
    return MakeErrorResult(
        -8642, fmt::format("Morphological reconstruction store sizes must match the expected image volume ({} values for dimensions {} x {} x {}). Marker size: {}; mask size: {}; output size: {}.",
                           vol, dimX, dimY, dimZ, m_Marker.getSize(), m_Mask.getSize(), m_Out.getSize()));
  }
  if(dimZ == 1)
  {
    const usize max2DBufferValues = m_MaxSlabValues;
    if constexpr(!UseTemporaryWork)
    {
      return run2D<Dilation>(m_Out, m_Mask, dimX, dimY, vol, max2DBufferValues);
    }
    else
    {
      auto fixedPlanResult = detail::CreateSweep2DPlan(dimX, dimY, m_Fixed2DBufferValues, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
      if(m_Fixed2DWorkValues > 0 && m_Fixed2DBufferValues > 0 && m_Mask.getStoreType() == IDataStore::StoreType::OutOfCore && vol <= m_Fixed2DWorkValues && fixedPlanResult.valid() &&
         fixedPlanResult.value().fullWidth)
      {
        auto fixedStoreResult = detail::BoundedMemorySweepStore<T>::Create(vol, m_Fixed2DWorkValues);
        if(fixedStoreResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(fixedStoreResult));
        }
        std::unique_ptr<detail::BoundedMemorySweepStore<T>> fixedStore = std::move(fixedStoreResult.value());
        if(Result<> r = run2D<Dilation>(*fixedStore, m_Mask, dimX, dimY, vol, m_Fixed2DBufferValues); r.invalid())
        {
          return r;
        }
        if(m_ShouldCancel)
        {
          return {};
        }
        return copyStore(*fixedStore, m_Out, vol, std::min(vol, m_MaxSlabValues));
      }
      if(!DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
      {
        return run2D<Dilation>(m_Out, m_Mask, dimX, dimY, vol, max2DBufferValues);
      }
      const usize maxBatchValues = std::min(vol, max2DBufferValues);
      auto workResult = detail::CreateSweepTemporaryStore<T>(vol, maxBatchValues, m_ShouldCancel, "Morphological reconstruction 2-D work");
      if(workResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(workResult));
      }
      std::unique_ptr<detail::SweepTemporaryStore<T>> workStore = std::move(workResult.value());
      if(Result<> r = run2D<Dilation>(*workStore, m_Mask, dimX, dimY, vol, max2DBufferValues); r.invalid())
      {
        return r;
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      return copyStore(*workStore, m_Out, vol, maxBatchValues);
    }
  }
  if constexpr(UseTemporaryWork)
  {
    if(m_UseResidentFullSweep && vol <= m_MaxSlabValues)
    {
      try
      {
        // Routes to the wavefront-parallel counterpart rather than the serial runResident3D. MorphologicalReconstructionSweepSerial.cpp
        // is left untouched by this routing change: runResident3D and runDirect stay compiled, byte-identical, and serve as the
        // reference implementations runResident3DWavefront and runDirectWavefront were mechanically derived from.
        return runResident3DWavefront<Dilation>(dimX, dimY, dimZ, sliceValues, vol);
      } catch(const std::bad_alloc&)
      {
        // The reservation is a hard upper bound, but the process allocator can still reject a request. Keep the
        // existing external temporary-store path as the safe fallback.
      }
    }
  }
  if constexpr(!UseTemporaryWork)
  {
    // Routes to the wavefront-parallel counterpart rather than the serial runDirect; see the routing comment above.
    return runDirectWavefront<Dilation>(dimX, dimY, dimZ, sliceValues, vol);
  }
  else
  {
    auto reconstruct = [&]<class WorkStore, class MaskStore>(WorkStore& workStore, const MaskStore& maskStore, bool initializeWork, usize sweepPairCount, bool& converged) -> Result<> {
      const int64 nX = static_cast<int64>(dimX);
      const int64 nY = static_cast<int64>(dimY);
      const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
      // Block/wave layout depends only on (dimX, dimY), which are fixed for the whole reconstruction, so this is
      // built once and shared by every plane, slab, direction, and sweep pair below.
      const detail::ReconstructionPlaneWavefrontSchedule planeWavefrontSchedule = detail::BuildReconstructionPlaneWavefrontSchedule(dimX, dimY);

      // At least one plane must fit so exceptionally wide planes remain supported; in that case a single working
      // plane can necessarily exceed the nominal byte target. Every other slab stays at or below the value budget.
      const usize slabPlanes = std::min(dimZ, std::max<usize>(1, m_MaxSlabValues / sliceValues));
      const usize slabCapacity = slabPlanes * sliceValues;
      auto workSlab = std::make_unique<T[]>(slabCapacity);
      auto maskSlab = std::make_unique<T[]>(slabCapacity);

      if(initializeWork)
      {
        // Initialize the selected working store to the marker with the same bounded slab buffer.
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
          if(Result<> r = workStore.copyFromBuffer(start, nonstd::span<const T>(workSlab.get(), count)); r.invalid())
          {
            return r;
          }
        }
      }

      // One boundary plane carries the last processed plane between slabs.
      auto adjacent = std::make_unique<T[]>(sliceValues);

      auto flat2d = [dimX](int64 x, int64 y) { return static_cast<usize>(y * static_cast<int64>(dimX) + x); };
      auto inBoundsXY = [nX, nY](int64 x, int64 y) { return x >= 0 && x < nX && y >= 0 && y < nY; };

      // Per-plane, per-direction convergence tracking: reconstruction localizes as it converges, so late sweep
      // pairs mostly re-touch a small active region while the rest of the volume has already reached its fixpoint.
      // planeDirty[z] is true whenever direction d MUST reprocess plane z because either its own stored contents,
      // or the boundary it receives from its predecessor in direction d's visitation order, may have changed since
      // d last visited it; a sweep may treat plane z as a proven no-op only while it is false. Tracking is per
      // PLANE (bounded by dimZ) rather than per fixed-size slab because the forward and reverse loops below tile
      // dimZ differently whenever dimZ is not an exact multiple of slabPlanes (the undersized remainder slab lands
      // at the volume's tail for forward and at its head for reverse); per-plane flags stay correct under either
      // tiling, and the actual SKIP decision below is still made per whole slab, matching the existing I/O
      // batching. All flags start dirty so the first sweep pair always processes the entire volume.
      std::vector<bool> forwardPlaneDirty(dimZ, true);
      std::vector<bool> reversePlaneDirty(dimZ, true);

      // A single directional sweep. Forward visits slabs and voxels ascending and uses PREVIOUS neighbors; reverse
      // visits them descending and uses LATER neighbors. Store I/O is serial and contiguous. All updates remain
      // sequential because later voxels read directly from the already-updated work slab.
      auto sweep = [&](bool forward, bool& changed) -> Result<> {
        const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
        std::vector<bool>& planeDirty = forward ? forwardPlaneDirty : reversePlaneDirty;
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
          const bool haveFollowingSlab = slabIndex + currentSlabPlanes < dimZ;

          // A slab is a provable no-op this sweep iff every plane it covers is already clean for this direction.
          // Skip its read, compute, and write entirely; the only cost is the one-plane boundary read below, needed
          // so a later, non-skippable slab in this same sweep still receives a correct carry-in.
          bool slabClean = true;
          for(usize z = zBegin; z < zBegin + currentSlabPlanes; ++z)
          {
            if(planeDirty[z])
            {
              slabClean = false;
              break;
            }
          }
          if(slabClean)
          {
            if(haveFollowingSlab)
            {
              const usize boundaryPlane = forward ? (zBegin + currentSlabPlanes - 1) : zBegin;
              if(Result<> r = workStore.copyIntoBuffer(boundaryPlane * sliceValues, nonstd::span<T>(adjacent.get(), sliceValues)); r.invalid())
              {
                return r;
              }
              haveAdjacent = true;
            }
            continue;
          }

          if(Result<> r = workStore.copyIntoBuffer(slabStart, nonstd::span<T>(workSlab.get(), currentSlabValues)); r.invalid())
          {
            return r;
          }
          if(Result<> r = maskStore.copyIntoBuffer(slabStart, nonstd::span<T>(maskSlab.get(), currentSlabValues)); r.invalid())
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

            // Processes one wavefront block: the identical per-voxel body the un-blocked sweep used, restricted to
            // the block's row/column extent and visited in the same forward/reverse row-major order. Independent
            // blocks within a wavefront run concurrently (see RunPlaneWavefront's bit-exactness proof); this
            // lambda itself always runs single-threaded over its own disjoint block.
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
                  const usize colIndex = forward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
                  const int64 x = static_cast<int64>(colIndex);
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
                    blockChanged = true;
                  }
                }
              }
              return blockChanged;
            };

            const bool planeChanged = detail::RunPlaneWavefront(planeWavefrontSchedule, forward, processBlock);
            if(planeChanged)
            {
              changed = true;
              slabChanged = true;
            }
            const usize z = zBegin + localZ;
            if(planeChanged)
            {
              // This plane's contents changed, so both directions must reconsider it the next time they reach it,
              // and whichever plane now receives its carry (this plane's successor along each direction's own
              // order) must be reconsidered too, since that carry just changed.
              forwardPlaneDirty[z] = true;
              reversePlaneDirty[z] = true;
              if(z + 1 < dimZ)
              {
                forwardPlaneDirty[z + 1] = true;
              }
              if(z > 0)
              {
                reversePlaneDirty[z - 1] = true;
              }
            }
            else
            {
              // Nothing changed under the carry-in this direction just used, so this direction can trust the
              // plane -- and the carry it hands onward -- unchanged until something invalidates it again.
              planeDirty[z] = false;
            }
          }
          // Capture a boundary plane only when another slab follows. Forward carries the slab's last plane; reverse
          // carries its first plane. The final slab has no downstream consumer for this copy.
          if(haveFollowingSlab)
          {
            const usize boundaryOffset = forward ? ((currentSlabPlanes - 1) * sliceValues) : 0;
            std::copy_n(workSlab.get() + boundaryOffset, sliceValues, adjacent.get());
            haveAdjacent = true;
          }
          if(slabChanged)
          {
            if(Result<> r = workStore.copyFromBuffer(slabStart, nonstd::span<const T>(workSlab.get(), currentSlabValues)); r.invalid())
            {
              return r;
            }
          }
        }
        return {};
      };

      converged = false;
      for(usize sweepPair = 0; sweepPair < sweepPairCount; ++sweepPair)
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
          converged = true;
          break; // reached the fixpoint
        }
      }
      return {};
    };

    bool converged = false;
    auto persistentPlanResult = detail::CreateReconstructionPersistentPrefixPlan<T>(m_Dims, m_MaxSlabValues);
    if(persistentPlanResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(persistentPlanResult));
    }
    const detail::ReconstructionPersistentPrefixPlan persistentPlan = persistentPlanResult.value();
    const bool isBorderMarker = m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMinimumInterior || m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMaximumInterior;
    const bool usePersistentPrefix = isBorderMarker && persistentPlan.residentPlanes > 0;
    if(!DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
    {
      if(usePersistentPrefix)
      {
        try
        {
          return reconstructPersistentPrefix<Dilation>(m_Out, m_Mask, dimX, dimY, dimZ, sliceValues, vol, persistentPlan, true);
        } catch(const std::bad_alloc&)
        {
          // The adaptive prefix is optional. The original bounded sweep below remains the allocation fallback.
        }
      }
      return reconstruct(m_Out, m_Mask, true, m_MaxSweepPairs, converged);
    }

    const usize maxBatchValues = std::min(vol, m_MaxSlabValues);
    auto workResult = detail::CreateSweepTemporaryStore<T>(vol, maxBatchValues, m_ShouldCancel, "Morphological reconstruction work");
    if(workResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(workResult));
    }
    std::unique_ptr<detail::SweepTemporaryStore<T>> workStore = std::move(workResult.value());

    auto copyStore = [&](const auto& sourceStore, auto& destinationStore) -> Result<> {
      auto buffer = std::make_unique<T[]>(maxBatchValues);
      for(usize start = 0; start < vol; start += maxBatchValues)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize count = std::min(maxBatchValues, vol - start);
        if(Result<> r = sourceStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
        {
          return r;
        }
        if(Result<> r = destinationStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
        {
          return r;
        }
      }
      return {};
    };

    bool usedPersistentPrefix = false;
    if(usePersistentPrefix)
    {
      try
      {
        if(Result<> result = reconstructPersistentPrefix<Dilation>(*workStore, m_Mask, dimX, dimY, dimZ, sliceValues, vol, persistentPlan, true); result.invalid())
        {
          return result;
        }
        usedPersistentPrefix = true;
      } catch(const std::bad_alloc&)
      {
        // The adaptive prefix is optional. The original bounded sweep below remains the allocation fallback.
      }
    }
    if(!usedPersistentPrefix)
    {
      if(Result<> result = reconstruct(*workStore, m_Mask, true, m_MaxSweepPairs, converged); result.invalid())
      {
        return result;
      }
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    if(usedPersistentPrefix)
    {
      return copyStoreRange(*workStore, m_Out, persistentPlan.residentValues, vol - persistentPlan.residentValues, maxBatchValues);
    }
    return copyStore(*workStore, m_Out);
  }
}

// runResident3DWavefront and runDirectWavefront below are ODR-used only from ReconstructSweep::runImpl, defined
// earlier in this same translation unit, so -- like reconstructPersistentPrefix and run2D, whose no-separate-
// directive reasoning is spelled out in the class-level explicit-instantiation comment near the bottom of this
// file -- their implicit instantiation resolves entirely within this TU wherever the runImpl instantiations at the
// bottom require them; they need neither a separate explicit-instantiation directive nor an export macro.
template <class T, bool UseTemporaryWork>
template <bool Dilation>
Result<> ReconstructSweep<T, UseTemporaryWork>::runResident3DWavefront(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol)
{
  using Tr = detail::ReconTraits<T, Dilation>;

  // Capacity comes from the active shared-budget reservation, identical to runResident3D.
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

  // Identical fast path to runResident3D: the frontier flood itself is untouched serial code (defined in
  // MorphologicalReconstructionSweepSerial.cpp), reached here through the existing cross-member instantiation.
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

  // Block/wave layout depends only on (dimX, dimY), fixed for the whole reconstruction, so it is built once and
  // shared by every plane, direction, and sweep pair below. Used for BOTH connectivities here -- unlike
  // ReconstructVincent's one-pass phases -- because this sweep always loops forward/reverse pairs to convergence;
  // see runResident3DWavefront's declaration in MorphologicalReconstructionEngine.hpp for the full justification.
  const detail::ReconstructionPlaneWavefrontSchedule planeWavefrontSchedule = detail::BuildReconstructionPlaneWavefrontSchedule(dimX, dimY);

  auto sweep = [&](bool forward, bool& changed) {
    const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
    for(usize planeIndex = 0; planeIndex < dimZ; ++planeIndex)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize z = forward ? planeIndex : dimZ - 1 - planeIndex;

      // Processes one wavefront block: the identical per-voxel body runResident3D's un-blocked sweep used,
      // restricted to the block's row/column extent and visited in the same forward/reverse row-major order.
      // Independent blocks within a wavefront run concurrently (see RunPlaneWavefront's documentation); this
      // lambda itself always runs single-threaded over its own disjoint block.
      auto processBlock = [&](const detail::ReconstructionPlaneBlock& block) -> bool {
        bool blockChanged = false;
        const usize rowSpan = block.rowEnd - block.rowBegin;
        const usize colSpan = block.colEnd - block.colBegin;
        for(usize localRow = 0; localRow < rowSpan; ++localRow)
        {
          const usize y = forward ? block.rowBegin + localRow : block.rowEnd - 1 - localRow;
          for(usize localCol = 0; localCol < colSpan; ++localCol)
          {
            const usize x = forward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
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
Result<> ReconstructSweep<T, UseTemporaryWork>::runDirectWavefront(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol)
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);

  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
  // Block/wave layout depends only on (dimX, dimY), fixed for the whole reconstruction, so it is built once and
  // shared by every plane, slab, direction, and sweep pair below. Used for BOTH connectivities for the same
  // convergence-loop reason documented on runResident3DWavefront above (this sweep also always loops
  // forward/reverse pairs to a fixed point).
  const detail::ReconstructionPlaneWavefrontSchedule planeWavefrontSchedule = detail::BuildReconstructionPlaneWavefrontSchedule(dimX, dimY);

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
  // neighbors; reverse visits them descending and uses LATER neighbors. Store I/O is serial and contiguous; each
  // plane's voxels are processed through the wavefront block schedule instead of runDirect's single-threaded
  // nested loop.
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

        // Processes one wavefront block: the identical per-voxel body runDirect's un-blocked sweep used,
        // restricted to the block's row/column extent and visited in the same forward/reverse row-major order.
        // Independent blocks within a wavefront run concurrently (see RunPlaneWavefront's documentation); this
        // lambda itself always runs single-threaded over its own disjoint block.
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
              const usize colIndex = forward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
              const int64 x = static_cast<int64>(colIndex);
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
                blockChanged = true;
              }
            }
          }
          return blockChanged;
        };

        if(detail::RunPlaneWavefront(planeWavefrontSchedule, forward, processBlock))
        {
          changed = true;
          slabChanged = true;
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

template <class T, bool UseTemporaryWork>
template <bool Dilation, class WorkStore, class MaskStore>
Result<> ReconstructSweep<T, UseTemporaryWork>::reconstructPersistentPrefix(WorkStore& workStore, const MaskStore& maskStore, usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol,
                                                                            const detail::ReconstructionPersistentPrefixPlan& plan, bool initializeWork)
{
  using Tr = detail::ReconTraits<T, Dilation>;
  const usize residentPlanes = plan.residentPlanes;
  const usize residentValues = plan.residentValues;
  const usize tailPlanes = dimZ - residentPlanes;
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);
  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);
  // Block/wave layout depends only on (dimX, dimY), which are fixed for the whole reconstruction, so this is
  // built once and shared by every resident and streamed plane, direction, and sweep pair below.
  const detail::ReconstructionPlaneWavefrontSchedule planeWavefrontSchedule = detail::BuildReconstructionPlaneWavefrontSchedule(dimX, dimY);

  auto residentWork = std::make_unique<T[]>(residentValues);
  auto streamingBuffers = std::make_unique<T[]>(3 * sliceValues);
  T* const workPlane = streamingBuffers.get();
  T* const maskPlane = streamingBuffers.get() + sliceValues;
  T* const adjacentPlane = streamingBuffers.get() + 2 * sliceValues;

  if(initializeWork)
  {
    if(Result<> result = loadMarker(0, nonstd::span<T>(residentWork.get(), residentValues)); result.invalid())
    {
      return result;
    }
    for(usize z = residentPlanes; z < dimZ; ++z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize planeStart = z * sliceValues;
      if(Result<> result = loadMarker(planeStart, nonstd::span<T>(workPlane, sliceValues)); result.invalid())
      {
        return result;
      }
      if(Result<> result = workStore.copyFromBuffer(planeStart, nonstd::span<const T>(workPlane, sliceValues)); result.invalid())
      {
        return result;
      }
    }
  }

  auto flat2d = [dimX](int64 x, int64 y) { return static_cast<usize>(y * static_cast<int64>(dimX) + x); };
  auto inBoundsXY = [nX, nY](int64 x, int64 y) { return x >= 0 && x < nX && y >= 0 && y < nY; };
  auto isPreviousOffset = [](const detail::ReconOffset& offset, bool xForward, bool yForward, bool zForward) {
    if(offset.dz != 0)
    {
      return zForward ? offset.dz < 0 : offset.dz > 0;
    }
    if(offset.dy != 0)
    {
      return yForward ? offset.dy < 0 : offset.dy > 0;
    }
    return xForward ? offset.dx < 0 : offset.dx > 0;
  };

  auto scanResidentPlane = [&](usize z, bool xForward, bool yForward, bool zForward, const T* currentMask, const T* outsideAdjacent, bool& changed) {
    const usize planeOffset = z * sliceValues;
    // Both call sites below always pass xForward == yForward (one uniform direction per call), which is what the
    // wavefront schedule assumes: the in-plane predecessor set (see RunPlaneWavefront) only mirrors correctly
    // between forward and reverse when row and column traversal reverse together.
    auto processBlock = [&](const detail::ReconstructionPlaneBlock& block) -> bool {
      bool blockChanged = false;
      const usize rowSpan = block.rowEnd - block.rowBegin;
      const usize colSpan = block.colEnd - block.colBegin;
      for(usize localRow = 0; localRow < rowSpan; ++localRow)
      {
        const usize rowIndex = yForward ? block.rowBegin + localRow : block.rowEnd - 1 - localRow;
        const int64 y = static_cast<int64>(rowIndex);
        for(usize localCol = 0; localCol < colSpan; ++localCol)
        {
          const usize columnIndex = xForward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
          const int64 x = static_cast<int64>(columnIndex);
          const usize position2d = flat2d(x, y);
          const usize position = planeOffset + position2d;
          T value = residentWork[position];
          if(!m_FullyConnected)
          {
            if(zForward)
            {
              if(z > 0)
              {
                value = Tr::fold(value, residentWork[position - sliceValues]);
              }
            }
            else if(z + 1 < residentPlanes)
            {
              value = Tr::fold(value, residentWork[position + sliceValues]);
            }
            else if(outsideAdjacent != nullptr)
            {
              value = Tr::fold(value, outsideAdjacent[position2d]);
            }
            if(yForward)
            {
              if(y > 0)
              {
                value = Tr::fold(value, residentWork[position - dimX]);
              }
            }
            else if(y + 1 < nY)
            {
              value = Tr::fold(value, residentWork[position + dimX]);
            }
            if(xForward)
            {
              if(x > 0)
              {
                value = Tr::fold(value, residentWork[position - 1]);
              }
            }
            else if(x + 1 < nX)
            {
              value = Tr::fold(value, residentWork[position + 1]);
            }
          }
          else
          {
            for(const detail::ReconOffset& offset : offsets.all)
            {
              if(!isPreviousOffset(offset, xForward, yForward, zForward))
              {
                continue;
              }
              const int64 neighborX = x + offset.dx;
              const int64 neighborY = y + offset.dy;
              const int64 neighborZ = static_cast<int64>(z) + offset.dz;
              if(!inBoundsXY(neighborX, neighborY) || neighborZ < 0 || neighborZ >= nZ)
              {
                continue;
              }
              const usize neighbor2d = flat2d(neighborX, neighborY);
              if(neighborZ < static_cast<int64>(residentPlanes))
              {
                value = Tr::fold(value, residentWork[static_cast<usize>(neighborZ) * sliceValues + neighbor2d]);
              }
              else if(outsideAdjacent != nullptr)
              {
                value = Tr::fold(value, outsideAdjacent[neighbor2d]);
              }
            }
          }
          const T newValue = Tr::clampToMask(value, currentMask[position2d]);
          if(newValue != residentWork[position])
          {
            residentWork[position] = newValue;
            blockChanged = true;
          }
        }
      }
      return blockChanged;
    };
    if(detail::RunPlaneWavefront(planeWavefrontSchedule, yForward, processBlock))
    {
      changed = true;
    }
  };

  auto scanStreamedPlane = [&](usize z, bool xForward, bool yForward, bool zForward, const T* adjacent, bool& changed, bool& planeChanged) {
    // Both call sites below always pass xForward == yForward, matching scanResidentPlane's assumption above.
    auto processBlock = [&](const detail::ReconstructionPlaneBlock& block) -> bool {
      bool blockChanged = false;
      const usize rowSpan = block.rowEnd - block.rowBegin;
      const usize colSpan = block.colEnd - block.colBegin;
      for(usize localRow = 0; localRow < rowSpan; ++localRow)
      {
        const usize rowIndex = yForward ? block.rowBegin + localRow : block.rowEnd - 1 - localRow;
        const int64 y = static_cast<int64>(rowIndex);
        for(usize localCol = 0; localCol < colSpan; ++localCol)
        {
          const usize columnIndex = xForward ? block.colBegin + localCol : block.colEnd - 1 - localCol;
          const int64 x = static_cast<int64>(columnIndex);
          const usize position = flat2d(x, y);
          T value = workPlane[position];
          if(!m_FullyConnected)
          {
            if(zForward)
            {
              if(z > 0 && adjacent != nullptr)
              {
                value = Tr::fold(value, adjacent[position]);
              }
            }
            else if(z + 1 < dimZ && adjacent != nullptr)
            {
              value = Tr::fold(value, adjacent[position]);
            }
            if(yForward)
            {
              if(y > 0)
              {
                value = Tr::fold(value, workPlane[position - dimX]);
              }
            }
            else if(y + 1 < nY)
            {
              value = Tr::fold(value, workPlane[position + dimX]);
            }
            if(xForward)
            {
              if(x > 0)
              {
                value = Tr::fold(value, workPlane[position - 1]);
              }
            }
            else if(x + 1 < nX)
            {
              value = Tr::fold(value, workPlane[position + 1]);
            }
          }
          else
          {
            for(const detail::ReconOffset& offset : offsets.all)
            {
              if(!isPreviousOffset(offset, xForward, yForward, zForward))
              {
                continue;
              }
              const int64 neighborX = x + offset.dx;
              const int64 neighborY = y + offset.dy;
              const int64 neighborZ = static_cast<int64>(z) + offset.dz;
              if(!inBoundsXY(neighborX, neighborY) || neighborZ < 0 || neighborZ >= nZ)
              {
                continue;
              }
              const usize neighbor2d = flat2d(neighborX, neighborY);
              value = Tr::fold(value, offset.dz == 0 ? workPlane[neighbor2d] : adjacent[neighbor2d]);
            }
          }
          const T newValue = Tr::clampToMask(value, maskPlane[position]);
          if(newValue != workPlane[position])
          {
            workPlane[position] = newValue;
            blockChanged = true;
          }
        }
      }
      return blockChanged;
    };
    if(detail::RunPlaneWavefront(planeWavefrontSchedule, yForward, processBlock))
    {
      changed = true;
      planeChanged = true;
    }
  };

  // Per-plane, per-direction convergence tracking, identical in spirit to the streamed sweep above: dirty[z] is
  // true whenever direction d must reprocess plane z because its own stored contents or the carry it receives
  // from its predecessor (in that direction's z-order) may have changed since d last visited it. The resident
  // prefix and the streamed tail share one z-indexed pair of flags because both traverse the same single z-order
  // (ascending 0..dimZ-1 for forward, descending for reverse); a resident plane's "carry" is simply its own
  // up-to-date value already sitting in residentWork, so skipping a clean resident batch needs no store read at
  // all, while skipping a clean streamed tail plane still needs the one bounded single-plane read that keeps
  // adjacentPlane correct for the next plane in that direction's order. All flags start dirty (rule: process
  // everything at least once).
  std::vector<bool> forwardPlaneDirty(dimZ, true);
  std::vector<bool> reversePlaneDirty(dimZ, true);
  // Records that plane z's contents just changed, so both directions must reconsider z, and whichever plane now
  // receives z's carry (z's successor along each direction's own order) must be reconsidered too.
  auto markPlaneChanged = [&](usize z) {
    forwardPlaneDirty[z] = true;
    reversePlaneDirty[z] = true;
    if(z + 1 < dimZ)
    {
      forwardPlaneDirty[z + 1] = true;
    }
    if(z > 0)
    {
      reversePlaneDirty[z - 1] = true;
    }
  };

  for(usize sweepPair = 0; sweepPair < m_MaxSweepPairs; ++sweepPair)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    bool changed = false;

    constexpr usize forwardMaskBatchPlanes = 3;
    for(usize batchBegin = 0; batchBegin < residentPlanes; batchBegin += forwardMaskBatchPlanes)
    {
      const usize batchPlanes = std::min(forwardMaskBatchPlanes, residentPlanes - batchBegin);
      bool batchClean = true;
      for(usize z = batchBegin; z < batchBegin + batchPlanes; ++z)
      {
        if(forwardPlaneDirty[z])
        {
          batchClean = false;
          break;
        }
      }
      if(batchClean)
      {
        // residentWork already holds the correct values for every plane in this batch; no mask read or
        // recompute is needed, and there is no store-backed carry to refresh (the "carry" is residentWork itself).
        continue;
      }
      if(Result<> result = maskStore.copyIntoBuffer(batchBegin * sliceValues, nonstd::span<T>(streamingBuffers.get(), batchPlanes * sliceValues)); result.invalid())
      {
        return result;
      }
      for(usize localPlane = 0; localPlane < batchPlanes; ++localPlane)
      {
        const usize z = batchBegin + localPlane;
        bool planeChanged = false;
        scanResidentPlane(z, true, true, true, streamingBuffers.get() + localPlane * sliceValues, nullptr, planeChanged);
        changed = changed || planeChanged;
        if(planeChanged)
        {
          markPlaneChanged(z);
        }
        else
        {
          forwardPlaneDirty[z] = false;
        }
      }
    }

    std::copy_n(residentWork.get() + (residentPlanes - 1) * sliceValues, sliceValues, adjacentPlane);
    for(usize z = residentPlanes; z < dimZ; ++z)
    {
      const usize planeStart = z * sliceValues;
      if(!forwardPlaneDirty[z])
      {
        // Provable no-op: refresh the carry for the next tail plane with a single bounded read instead of the
        // full read-compute-write cycle.
        if(Result<> result = workStore.copyIntoBuffer(planeStart, nonstd::span<T>(adjacentPlane, sliceValues)); result.invalid())
        {
          return result;
        }
        continue;
      }
      if(Result<> result = workStore.copyIntoBuffer(planeStart, nonstd::span<T>(workPlane, sliceValues)); result.invalid())
      {
        return result;
      }
      if(Result<> result = maskStore.copyIntoBuffer(planeStart, nonstd::span<T>(maskPlane, sliceValues)); result.invalid())
      {
        return result;
      }
      bool planeChanged = false;
      scanStreamedPlane(z, true, true, true, adjacentPlane, changed, planeChanged);
      if(planeChanged)
      {
        if(Result<> result = workStore.copyFromBuffer(planeStart, nonstd::span<const T>(workPlane, sliceValues)); result.invalid())
        {
          return result;
        }
        markPlaneChanged(z);
      }
      else
      {
        forwardPlaneDirty[z] = false;
      }
      std::copy_n(workPlane, sliceValues, adjacentPlane);
    }

    bool haveAdjacent = false;
    for(usize planeIndex = 0; planeIndex < tailPlanes; ++planeIndex)
    {
      const usize z = dimZ - 1 - planeIndex;
      const usize planeStart = z * sliceValues;
      if(!reversePlaneDirty[z])
      {
        if(Result<> result = workStore.copyIntoBuffer(planeStart, nonstd::span<T>(adjacentPlane, sliceValues)); result.invalid())
        {
          return result;
        }
        haveAdjacent = true;
        continue;
      }
      if(Result<> result = workStore.copyIntoBuffer(planeStart, nonstd::span<T>(workPlane, sliceValues)); result.invalid())
      {
        return result;
      }
      if(Result<> result = maskStore.copyIntoBuffer(planeStart, nonstd::span<T>(maskPlane, sliceValues)); result.invalid())
      {
        return result;
      }
      bool planeChanged = false;
      scanStreamedPlane(z, false, false, false, haveAdjacent ? adjacentPlane : nullptr, changed, planeChanged);
      if(planeChanged)
      {
        if(Result<> result = workStore.copyFromBuffer(planeStart, nonstd::span<const T>(workPlane, sliceValues)); result.invalid())
        {
          return result;
        }
        markPlaneChanged(z);
      }
      else
      {
        reversePlaneDirty[z] = false;
      }
      std::copy_n(workPlane, sliceValues, adjacentPlane);
      haveAdjacent = true;
    }

    constexpr usize reverseMaskBatchPlanes = 2;
    for(usize processedPlanes = 0; processedPlanes < residentPlanes; processedPlanes += reverseMaskBatchPlanes)
    {
      const usize batchEnd = residentPlanes - processedPlanes;
      const usize batchPlanes = std::min(reverseMaskBatchPlanes, batchEnd);
      const usize batchBegin = batchEnd - batchPlanes;
      bool batchClean = true;
      for(usize z = batchBegin; z < batchEnd; ++z)
      {
        if(reversePlaneDirty[z])
        {
          batchClean = false;
          break;
        }
      }
      if(batchClean)
      {
        continue;
      }
      if(Result<> result = maskStore.copyIntoBuffer(batchBegin * sliceValues, nonstd::span<T>(streamingBuffers.get(), batchPlanes * sliceValues)); result.invalid())
      {
        return result;
      }
      for(usize localPlane = 0; localPlane < batchPlanes; ++localPlane)
      {
        const usize z = batchEnd - 1 - localPlane;
        const usize maskOffset = z - batchBegin;
        bool planeChanged = false;
        scanResidentPlane(z, false, false, false, streamingBuffers.get() + maskOffset * sliceValues, z + 1 == residentPlanes ? adjacentPlane : nullptr, planeChanged);
        changed = changed || planeChanged;
        if(planeChanged)
        {
          markPlaneChanged(z);
        }
        else
        {
          reversePlaneDirty[z] = false;
        }
      }
    }

    if(!changed)
    {
      break;
    }
  }
  if(m_ShouldCancel)
    if(m_ShouldCancel)
    {
      return {};
    }
  if(Result<> result = m_Out.copyFromBuffer(0, nonstd::span<const T>(residentWork.get(), residentValues)); result.invalid())
  {
    return result;
  }
  return {};
}

template <class T, bool UseTemporaryWork>
template <bool Dilation, class WorkStore, class MaskStore>
Result<> ReconstructSweep<T, UseTemporaryWork>::run2D(WorkStore& workStore, const MaskStore& maskStore, usize dimX, usize dimY, usize vol, usize maxBufferValues)
{
  using Tr = detail::ReconTraits<T, Dilation>;
  auto planResult = detail::CreateSweep2DPlan(dimX, dimY, maxBufferValues, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(planResult));
  }
  detail::Sweep2DPlan plan = planResult.value();
  if(plan.fullWidth)
  {
    const std::optional<ShapeType> chunkShape = workStore.getChunkShape();
    if(chunkShape.has_value() && chunkShape->size() >= 3 && (*chunkShape)[0] == 1 && (*chunkShape)[1] > 0 && (*chunkShape)[1] <= plan.coreRows && (*chunkShape)[2] == dimX)
    {
      plan.coreRows = (plan.coreRows / (*chunkShape)[1]) * (*chunkShape)[1];
    }
  }
  const detail::ReconOffsets offsets = detail::MakeReconstructionOffsets(m_FullyConnected);

  if(plan.fullWidth)
  {
    const usize maximumReadRows = std::min(dimY, plan.coreRows + 1);
    const usize maximumReadValues = maximumReadRows * dimX;
    auto workBuffer = std::make_unique<T[]>(maximumReadValues);
    auto maskBuffer = std::make_unique<T[]>(plan.coreRows * dimX);

    for(usize coreBegin = 0; coreBegin < dimY; coreBegin += plan.coreRows)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize coreRows = std::min(plan.coreRows, dimY - coreBegin);
      const usize coreValues = coreRows * dimX;
      const usize coreStart = coreBegin * dimX;
      if(Result<> r = loadMarker(coreStart, nonstd::span<T>(workBuffer.get(), coreValues)); r.invalid())
      {
        return r;
      }
      if(Result<> r = workStore.copyFromBuffer(coreStart, nonstd::span<const T>(workBuffer.get(), coreValues)); r.invalid())
      {
        return r;
      }
    }

    const usize blockCount = (dimY - 1) / plan.coreRows + 1;
    bool cacheValid = false;
    usize cachedCoreBegin = 0;
    usize cachedCoreRows = 0;
    usize cachedReadBegin = 0;
    usize cachedReadEnd = 0;
    auto sweep = [&](bool forward, bool& changed) -> Result<> {
      const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
      for(usize blockIteration = 0; blockIteration < blockCount; ++blockIteration)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize blockIndex = forward ? blockIteration : blockCount - 1 - blockIteration;
        const usize coreBegin = blockIndex * plan.coreRows;
        const usize coreRows = std::min(plan.coreRows, dimY - coreBegin);
        const usize coreEnd = coreBegin + coreRows;
        const usize requiredReadBegin = forward && coreBegin > 0 ? coreBegin - 1 : coreBegin;
        const usize requiredReadEnd = !forward && coreEnd < dimY ? coreEnd + 1 : coreEnd;
        const bool reuseCachedBlock = cacheValid && cachedCoreBegin == coreBegin && cachedCoreRows == coreRows && cachedReadBegin <= requiredReadBegin && cachedReadEnd >= requiredReadEnd;
        const usize readBegin = reuseCachedBlock ? cachedReadBegin : requiredReadBegin;
        const usize readEnd = reuseCachedBlock ? cachedReadEnd : requiredReadEnd;
        const usize readValues = (readEnd - readBegin) * dimX;
        const usize coreValues = coreRows * dimX;
        if(!reuseCachedBlock)
        {
          if(Result<> r = workStore.copyIntoBuffer(readBegin * dimX, nonstd::span<T>(workBuffer.get(), readValues)); r.invalid())
          {
            return r;
          }
          if(Result<> r = maskStore.copyIntoBuffer(coreBegin * dimX, nonstd::span<T>(maskBuffer.get(), coreValues)); r.invalid())
          {
            return r;
          }
        }

        bool blockChanged = false;
        for(usize rowIndex = 0; rowIndex < coreRows; ++rowIndex)
        {
          const usize y = forward ? coreBegin + rowIndex : coreEnd - 1 - rowIndex;
          const usize localY = y - readBegin;
          const usize rowOffset = localY * dimX;
          const usize maskRowOffset = (y - coreBegin) * dimX;
          for(usize columnIndex = 0; columnIndex < dimX; ++columnIndex)
          {
            const usize x = forward ? columnIndex : dimX - 1 - columnIndex;
            const usize position = rowOffset + x;
            T value = workBuffer[position];
            if(!m_FullyConnected)
            {
              if(forward)
              {
                if(y > 0)
                {
                  value = Tr::fold(value, workBuffer[position - dimX]);
                }
                if(x > 0)
                {
                  value = Tr::fold(value, workBuffer[position - 1]);
                }
              }
              else
              {
                if(x + 1 < dimX)
                {
                  value = Tr::fold(value, workBuffer[position + 1]);
                }
                if(y + 1 < dimY)
                {
                  value = Tr::fold(value, workBuffer[position + dimX]);
                }
              }
            }
            else
            {
              for(const detail::ReconOffset& offset : half)
              {
                if(offset.dz != 0)
                {
                  continue;
                }
                const int64 neighborX = static_cast<int64>(x) + offset.dx;
                const int64 neighborY = static_cast<int64>(y) + offset.dy;
                if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
                {
                  continue;
                }
                const usize neighbor = (static_cast<usize>(neighborY) - readBegin) * dimX + static_cast<usize>(neighborX);
                value = Tr::fold(value, workBuffer[neighbor]);
              }
            }
            const T newValue = Tr::clampToMask(value, maskBuffer[maskRowOffset + x]);
            if(newValue != workBuffer[position])
            {
              workBuffer[position] = newValue;
              changed = true;
              blockChanged = true;
            }
          }
        }
        if(blockChanged)
        {
          const usize coreOffset = (coreBegin - readBegin) * dimX;
          if(Result<> r = workStore.copyFromBuffer(coreBegin * dimX, nonstd::span<const T>(workBuffer.get() + coreOffset, coreValues)); r.invalid())
          {
            return r;
          }
        }
        cacheValid = true;
        cachedCoreBegin = coreBegin;
        cachedCoreRows = coreRows;
        cachedReadBegin = readBegin;
        cachedReadEnd = readEnd;
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
      if(Result<> r = sweep(false, changed); r.invalid())
      {
        return r;
      }
      if(!changed)
      {
        break;
      }
    }
    return {};
  }

  auto currentBuffer = std::make_unique<T[]>(plan.coreColumns);
  auto maskBuffer = std::make_unique<T[]>(plan.coreColumns);
  auto adjacentBuffer = std::make_unique<T[]>(plan.coreColumns + 2);
  for(usize y = 0; y < dimY; ++y)
  {
    for(usize xBegin = 0; xBegin < dimX; xBegin += plan.coreColumns)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize coreColumns = std::min(plan.coreColumns, dimX - xBegin);
      const usize start = y * dimX + xBegin;
      if(Result<> r = loadMarker(start, nonstd::span<T>(currentBuffer.get(), coreColumns)); r.invalid())
      {
        return r;
      }
      if(Result<> r = workStore.copyFromBuffer(start, nonstd::span<const T>(currentBuffer.get(), coreColumns)); r.invalid())
      {
        return r;
      }
    }
  }

  auto sweepTiled = [&](bool forward, bool& changed) -> Result<> {
    const std::vector<detail::ReconOffset>& half = forward ? offsets.previous : offsets.later;
    for(usize rowIndex = 0; rowIndex < dimY; ++rowIndex)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize y = forward ? rowIndex : dimY - 1 - rowIndex;
      bool haveHorizontalCarry = false;
      T horizontalCarry{};
      for(usize processedColumns = 0; processedColumns < dimX; processedColumns += plan.coreColumns)
      {
        const usize coreColumns = std::min(plan.coreColumns, dimX - processedColumns);
        const usize xBegin = forward ? processedColumns : dimX - processedColumns - coreColumns;
        const usize start = y * dimX + xBegin;
        if(Result<> r = workStore.copyIntoBuffer(start, nonstd::span<T>(currentBuffer.get(), coreColumns)); r.invalid())
        {
          return r;
        }
        if(Result<> r = maskStore.copyIntoBuffer(start, nonstd::span<T>(maskBuffer.get(), coreColumns)); r.invalid())
        {
          return r;
        }

        const bool haveAdjacentRow = forward ? y > 0 : y + 1 < dimY;
        usize adjacentBegin = 0;
        if(haveAdjacentRow)
        {
          adjacentBegin = xBegin > 0 ? xBegin - 1 : xBegin;
          const usize adjacentEnd = std::min(dimX, xBegin + coreColumns + 1);
          const usize adjacentColumns = adjacentEnd - adjacentBegin;
          const usize adjacentY = forward ? y - 1 : y + 1;
          if(Result<> r = workStore.copyIntoBuffer(adjacentY * dimX + adjacentBegin, nonstd::span<T>(adjacentBuffer.get(), adjacentColumns)); r.invalid())
          {
            return r;
          }
        }

        bool tileChanged = false;
        for(usize columnIndex = 0; columnIndex < coreColumns; ++columnIndex)
        {
          const usize localX = forward ? columnIndex : coreColumns - 1 - columnIndex;
          const usize x = xBegin + localX;
          T value = currentBuffer[localX];
          if(!m_FullyConnected)
          {
            if(forward)
            {
              if(haveAdjacentRow)
              {
                value = Tr::fold(value, adjacentBuffer[x - adjacentBegin]);
              }
              if(localX > 0)
              {
                value = Tr::fold(value, currentBuffer[localX - 1]);
              }
              else if(haveHorizontalCarry)
              {
                value = Tr::fold(value, horizontalCarry);
              }
            }
            else if(localX + 1 < coreColumns)
            {
              value = Tr::fold(value, currentBuffer[localX + 1]);
            }
            else if(haveHorizontalCarry)
            {
              value = Tr::fold(value, horizontalCarry);
            }
            if(!forward && haveAdjacentRow)
            {
              value = Tr::fold(value, adjacentBuffer[x - adjacentBegin]);
            }
          }
          else
          {
            for(const detail::ReconOffset& offset : half)
            {
              if(offset.dz != 0)
              {
                continue;
              }
              const int64 neighborX = static_cast<int64>(x) + offset.dx;
              const int64 neighborY = static_cast<int64>(y) + offset.dy;
              if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
              {
                continue;
              }
              if(offset.dy != 0)
              {
                value = Tr::fold(value, adjacentBuffer[static_cast<usize>(neighborX) - adjacentBegin]);
              }
              else if(neighborX >= static_cast<int64>(xBegin) && neighborX < static_cast<int64>(xBegin + coreColumns))
              {
                value = Tr::fold(value, currentBuffer[static_cast<usize>(neighborX) - xBegin]);
              }
              else if(haveHorizontalCarry)
              {
                value = Tr::fold(value, horizontalCarry);
              }
            }
          }
          const T newValue = Tr::clampToMask(value, maskBuffer[localX]);
          if(newValue != currentBuffer[localX])
          {
            currentBuffer[localX] = newValue;
            changed = true;
            tileChanged = true;
          }
        }
        horizontalCarry = forward ? currentBuffer[coreColumns - 1] : currentBuffer[0];
        haveHorizontalCarry = true;
        if(tileChanged)
        {
          if(Result<> r = workStore.copyFromBuffer(start, nonstd::span<const T>(currentBuffer.get(), coreColumns)); r.invalid())
          {
            return r;
          }
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
    if(Result<> r = sweepTiled(true, changed); r.invalid())
    {
      return r;
    }
    if(Result<> r = sweepTiled(false, changed); r.invalid())
    {
      return r;
    }
    if(!changed)
    {
      break;
    }
  }
  return {};
}

// This class-level explicit instantiation covers ReconstructSweep's ordinary (non-template) members whose
// definitions are visible in this translation unit -- the constructor, destructor, assignment, operator(),
// copyStore, copyStoreRange, and loadMarker, all still defined inline in the class body in
// MorphologicalReconstructionEngine.hpp. Per the language rule for explicit class template instantiation, it does
// NOT reach reconstructPersistentPrefix, run2D, or runImpl, since all three are member templates; explicit class
// instantiation never implicitly instantiates member templates regardless of where they are defined.
// reconstructPersistentPrefix and run2D are called only from runImpl's body above, in this same translation unit,
// so they are instantiated implicitly wherever the runImpl instantiations below require them; no separate
// directive is needed for them, and (being reached only from within this same shared library) they need no
// export macro. operator() itself stays defined inline in the header (see MorphologicalReconstructionEngine.hpp)
// and calls runImpl<Dilation> directly, so runImpl is compiled into every consumer of that header, including
// other shared libraries (e.g. the ImageProcessing plugin). Each runImpl<Dilation> specialization a consumer can
// reach is therefore both explicitly instantiated and exported below, so a cross-library call through the
// header-inline operator() can resolve it at link time.
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int8, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int8, true>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint8, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint8, true>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int16, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int16, true>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint16, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint16, true>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int32, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int32, true>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint32, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint32, true>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int64, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int64, true>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint64, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint64, true>;

template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float32, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float32, true>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float64, false>;
template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float64, true>;

#define SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(T)                                                                                                                                              \
  template SIMPLNX_TEMPLATE_EXPORT Result<> ReconstructSweep<T, false>::runImpl<true>();                                                                                                               \
  template SIMPLNX_TEMPLATE_EXPORT Result<> ReconstructSweep<T, false>::runImpl<false>();                                                                                                              \
  template SIMPLNX_TEMPLATE_EXPORT Result<> ReconstructSweep<T, true>::runImpl<true>();                                                                                                                \
  template SIMPLNX_TEMPLATE_EXPORT Result<> ReconstructSweep<T, true>::runImpl<false>()

SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(int8);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(uint8);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(int16);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(uint16);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(int32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(uint32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(int64);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(uint64);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(float32);
SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL(float64);

#undef SIMPLNX_INSTANTIATE_RECONSTRUCT_SWEEP_RUN_IMPL
} // namespace nx::core::ImageProcessing
