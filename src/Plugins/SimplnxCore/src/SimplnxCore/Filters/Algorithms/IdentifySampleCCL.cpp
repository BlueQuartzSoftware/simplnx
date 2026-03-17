#include "IdentifySampleCCL.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
// =============================================================================
// runForwardCCL
// =============================================================================
// Generic chunk-sequential Connected Component Labeling function that works on
// any boolean condition. It processes the volume in chunk order (OOC-friendly)
// using a rolling 2-slice label buffer instead of storing labels for the
// entire volume.
//
// How it works:
//   - Scans voxels in chunk order (z, y, x innermost). For each voxel where
//     `condition(store, index)` returns true, checks three backward neighbors
//     (x-1, y-1, z-1) for existing labels.
//   - If no labeled neighbor exists, assigns a new provisional label.
//   - If multiple differently-labeled neighbors exist, unites them in the
//     union-find structure.
//   - Tracks per-label voxel counts (labelSizes) so the largest root can be
//     identified after flattening, without a separate counting pass.
//
// The `condition` lambda determines which voxels to label. For example:
//   - `store[idx] == true` labels good voxels (sample identification)
//   - `!store[idx]` labels bad voxels (hole detection)
//
// The lastClearedZ optimization prevents re-clearing the rolling buffer when
// a Z-slice spans multiple OOC chunks (e.g., chunk shape that splits within
// a Z-plane). Without it, entering the same Z from the next chunk would
// zero out labels already written by the previous chunk.
//
// Returns a CCLResult containing the union-find, accumulated root sizes,
// the next available label, and the largest root/size.
// =============================================================================
struct CCLResult
{
  VectorUnionFind unionFind;
  std::vector<uint64> rootSizes;
  int64 nextLabel = 1;
  int64 largestRoot = -1;
  uint64 largestSize = 0;
};

template <typename T, typename ConditionFn>
CCLResult runForwardCCL(AbstractDataStore<T>& store, int64 dimX, int64 dimY, int64 dimZ, ConditionFn condition, const std::atomic_bool& shouldCancel)
{
  CCLResult result;
  const usize sliceSize = static_cast<usize>(dimX * dimY);

  // Rolling 2-slice buffer: only the current and previous Z-slice labels are
  // kept in memory. The scanline CCL only looks at backward neighbors (x-1,
  // y-1, z-1), so two slices suffice. This gives O(dimX * dimY) memory
  // instead of O(volume).
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  // Per-label voxel count, accumulated during the forward scan so we can
  // find the largest component after flattening without a separate pass.
  std::vector<uint64> labelSizes;
  labelSizes.push_back(0); // index 0 unused (labels start at 1)

  const uint64 numChunks = store.getNumberOfChunks();
  // Track last cleared Z-slice to avoid re-clearing when a Z-slice spans
  // multiple chunks (see algorithm overview comment above).
  int64 lastClearedZ = -1;

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    if(shouldCancel)
    {
      return result;
    }
    store.loadChunk(chunkIdx);
    const auto lower = store.getChunkLowerBounds(chunkIdx);
    const auto upper = store.getChunkUpperBounds(chunkIdx);

    for(usize z = lower[0]; z <= upper[0]; z++)
    {
      // Clear current slice in rolling buffer only when entering a NEW z value.
      // A single Z-slice may span multiple chunks (e.g., chunk shape 1x3x25 with dimY=5),
      // so we must not re-clear data written by a previous chunk for the same z.
      const usize curOff = (z % 2) * sliceSize;
      if(static_cast<int64>(z) != lastClearedZ)
      {
        std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
        lastClearedZ = static_cast<int64>(z);
      }
      const usize prevOff = ((z + 1) % 2) * sliceSize;

      for(usize y = lower[1]; y <= upper[1]; y++)
      {
        for(usize x = lower[2]; x <= upper[2]; x++)
        {
          const usize index = z * sliceSize + y * static_cast<usize>(dimX) + x;

          if(!condition(store, index))
          {
            continue;
          }

          const usize inSlice = y * static_cast<usize>(dimX) + x;
          int64 nbrA = 0, nbrB = 0, nbrC = 0;

          if(x > 0)
          {
            nbrA = labelBuffer[curOff + inSlice - 1];
          }
          if(y > 0)
          {
            nbrB = labelBuffer[curOff + inSlice - static_cast<usize>(dimX)];
          }
          if(z > 0)
          {
            nbrC = labelBuffer[prevOff + inSlice];
          }

          int64 minLabel = 0;
          if(nbrA > 0)
          {
            minLabel = nbrA;
          }
          if(nbrB > 0 && (minLabel == 0 || nbrB < minLabel))
          {
            minLabel = nbrB;
          }
          if(nbrC > 0 && (minLabel == 0 || nbrC < minLabel))
          {
            minLabel = nbrC;
          }

          int64 assignedLabel;
          if(minLabel == 0)
          {
            assignedLabel = result.nextLabel++;
            result.unionFind.makeSet(assignedLabel);
            labelSizes.resize(result.nextLabel, 0);
          }
          else
          {
            assignedLabel = minLabel;
            if(nbrA > 0 && nbrA != assignedLabel)
            {
              result.unionFind.unite(assignedLabel, nbrA);
            }
            if(nbrB > 0 && nbrB != assignedLabel)
            {
              result.unionFind.unite(assignedLabel, nbrB);
            }
            if(nbrC > 0 && nbrC != assignedLabel)
            {
              result.unionFind.unite(assignedLabel, nbrC);
            }
          }

          labelBuffer[curOff + inSlice] = assignedLabel;
          labelSizes[assignedLabel]++;
        }
      }
    }
  }

  // Flatten union-find and accumulate sizes to roots
  result.rootSizes.resize(result.nextLabel, 0);
  for(int64 lbl = 1; lbl < result.nextLabel; lbl++)
  {
    int64 root = result.unionFind.find(lbl);
    result.rootSizes[root] += labelSizes[lbl];
  }

  // Find largest root
  for(int64 r = 1; r < result.nextLabel; r++)
  {
    if(result.rootSizes[r] >= result.largestSize)
    {
      result.largestSize = result.rootSizes[r];
      result.largestRoot = r;
    }
  }

  return result;
}

// =============================================================================
// replayForwardCCL
// =============================================================================
// Re-derives labels by running the exact same forward CCL scan a second time
// (same chunk order, same scanline traversal, same union-find). Since CCL
// label assignment is fully deterministic given the same scan order and
// condition, the re-derived provisional labels match the original ones from
// runForwardCCL exactly. The union-find (already flattened) is then used to
// resolve each provisional label to its root.
//
// The `action` lambda is called for each labeled voxel with its resolved root
// label, the store, and the voxel's (x, y, z) coordinates. This allows
// per-voxel decisions (e.g., "mask out if root != largestRoot", or "fill if
// root is an interior hole") without ever storing labels for the entire volume.
//
// This is the key OOC trick: by re-computing labels on the fly using only a
// 2-slice rolling buffer, we avoid O(volume) label storage. The trade-off is
// reading the data twice, but for OOC datasets the memory savings are critical.
//
// Note: the union-find unite() calls from the first pass are not repeated here
// because the union-find is already flattened. We only need the label
// assignment logic to re-derive the same provisional labels.
// =============================================================================
template <typename T, typename ConditionFn, typename ActionFn>
void replayForwardCCL(AbstractDataStore<T>& store, int64 dimX, int64 dimY, int64 dimZ, VectorUnionFind& unionFind, ConditionFn condition, ActionFn action, const std::atomic_bool& shouldCancel)
{
  const usize sliceSize = static_cast<usize>(dimX * dimY);
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  int64 nextLabel = 1;

  const uint64 numChunks = store.getNumberOfChunks();
  int64 lastClearedZ = -1;

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    if(shouldCancel)
    {
      return;
    }
    store.loadChunk(chunkIdx);
    const auto lower = store.getChunkLowerBounds(chunkIdx);
    const auto upper = store.getChunkUpperBounds(chunkIdx);

    for(usize z = lower[0]; z <= upper[0]; z++)
    {
      const usize curOff = (z % 2) * sliceSize;
      if(static_cast<int64>(z) != lastClearedZ)
      {
        std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
        lastClearedZ = static_cast<int64>(z);
      }
      const usize prevOff = ((z + 1) % 2) * sliceSize;

      for(usize y = lower[1]; y <= upper[1]; y++)
      {
        for(usize x = lower[2]; x <= upper[2]; x++)
        {
          const usize index = z * sliceSize + y * static_cast<usize>(dimX) + x;

          if(!condition(store, index))
          {
            continue;
          }

          const usize inSlice = y * static_cast<usize>(dimX) + x;
          int64 nbrA = 0, nbrB = 0, nbrC = 0;

          if(x > 0)
          {
            nbrA = labelBuffer[curOff + inSlice - 1];
          }
          if(y > 0)
          {
            nbrB = labelBuffer[curOff + inSlice - static_cast<usize>(dimX)];
          }
          if(z > 0)
          {
            nbrC = labelBuffer[prevOff + inSlice];
          }

          int64 minLabel = 0;
          if(nbrA > 0)
          {
            minLabel = nbrA;
          }
          if(nbrB > 0 && (minLabel == 0 || nbrB < minLabel))
          {
            minLabel = nbrB;
          }
          if(nbrC > 0 && (minLabel == 0 || nbrC < minLabel))
          {
            minLabel = nbrC;
          }

          int64 assignedLabel;
          if(minLabel == 0)
          {
            assignedLabel = nextLabel++;
          }
          else
          {
            assignedLabel = minLabel;
          }

          labelBuffer[curOff + inSlice] = assignedLabel;

          // Apply the action with the re-derived label
          int64 root = unionFind.find(assignedLabel);
          action(store, index, root, x, y, z);
        }
      }
    }
  }
}

// =============================================================================
// IdentifySampleCCLFunctor
// =============================================================================
// Chunk-sequential scanline CCL implementation for identifying the largest
// connected component of good voxels in a 3D image geometry, then optionally
// filling interior holes. Processes data in chunk order to avoid random chunk
// access in OOC mode, using a 2-slice rolling buffer (O(slice) memory) instead
// of O(volume).
//
// The algorithm has up to four phases:
//
// Phase 1: Forward CCL on good voxels
//   Run runForwardCCL with condition = (goodVoxels[idx] == true) to discover
//   all connected components and find the largest one by voxel count.
//
// Phase 2: Replay CCL to mask non-sample voxels
//   Run replayForwardCCL with the same good-voxel condition. For each voxel
//   whose resolved root != largestRoot, set goodVoxels to false. This removes
//   satellite regions and noise without storing the full label volume.
//
// Phase 3 (if fillHoles): Forward CCL on bad voxels
//   Run runForwardCCL with condition = (!goodVoxels[idx]) to discover all
//   connected components of non-sample space (potential holes + exterior).
//
// Phase 4 (if fillHoles): Replay CCL to identify and fill interior holes
//   First replay: for each bad-voxel component, check if any voxel lies on
//   a domain boundary. Mark boundary-touching roots in a boolean vector.
//   Second replay: for each bad voxel whose root is NOT boundary-touching,
//   set goodVoxels to true (filling the interior hole).
// =============================================================================
struct IdentifySampleCCLFunctor
{
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    SizeVec3 udims = imageGeom->getDimensions();
    const int64 dimX = static_cast<int64>(udims[0]);
    const int64 dimY = static_cast<int64>(udims[1]);
    const int64 dimZ = static_cast<int64>(udims[2]);

    const uint64 numChunks = goodVoxels.getNumberOfChunks();

    // --- Phase 1: Forward CCL on good voxels ----------------------------------
    // Discover all connected components of good voxels and find the largest one.
    // The condition lambda selects voxels where goodVoxels[idx] is true.
    messageHandler(IFilter::Message::Type::Info, "Identifying sample regions...");
    auto goodCondition = [](const AbstractDataStore<T>& s, usize idx) -> bool { return static_cast<bool>(s[idx]); };
    auto cclResult = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, goodCondition, shouldCancel);

    if(shouldCancel || cclResult.largestRoot < 0)
    {
      return;
    }

    // --- Phase 2: Replay CCL to mask non-sample voxels ----------------------
    // Re-derive labels using a second forward pass with the same scan order
    // and condition. For each voxel whose resolved root is not the largest
    // component, set goodVoxels to false (removing satellite regions/noise).
    // No O(volume) label storage is needed -- labels are recomputed on the fly.
    messageHandler(IFilter::Message::Type::Info, "Masking non-sample voxels...");
    const int64 largestRoot = cclResult.largestRoot;
    replayForwardCCL<T>(
        goodVoxels, dimX, dimY, dimZ, cclResult.unionFind, goodCondition,
        [&largestRoot](AbstractDataStore<T>& s, usize idx, int64 root, usize /*x*/, usize /*y*/, usize /*z*/) {
          if(root != largestRoot)
          {
            s.setValue(idx, static_cast<T>(false));
          }
        },
        shouldCancel);
    goodVoxels.flush();

    // --- Phase 3: Forward CCL on bad voxels (hole detection) -----------------
    // Only runs if fillHoles is true. Discovers connected components of
    // non-good voxels (the complement of the sample). These include both
    // exterior empty space and interior holes.
    if(fillHoles)
    {
      messageHandler(IFilter::Message::Type::Info, "Filling holes in sample...");

      // Condition selects voxels where goodVoxels[idx] is false (bad data)
      auto holeCondition = [](const AbstractDataStore<T>& s, usize idx) -> bool { return !static_cast<bool>(s[idx]); };
      auto holeCCL = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, holeCondition, shouldCancel);

      if(shouldCancel)
      {
        return;
      }

      // --- Phase 4a: Replay CCL to identify boundary-touching roots ---------
      // Replay the hole CCL to re-derive labels. For each labeled voxel,
      // check if it lies on a domain boundary face. If so, mark its resolved
      // root as boundary-touching. Components that touch the boundary are
      // exterior space (not holes). This avoids O(volume) label storage by
      // re-computing labels on the fly.
      std::vector<bool> boundaryRoots(holeCCL.nextLabel, false);
      replayForwardCCL<T>(
          goodVoxels, dimX, dimY, dimZ, holeCCL.unionFind, holeCondition,
          [&boundaryRoots, dimX, dimY, dimZ](AbstractDataStore<T>& /*s*/, usize /*idx*/, int64 root, usize x, usize y, usize z) {
            if(x == 0 || x == static_cast<usize>(dimX - 1) || y == 0 || y == static_cast<usize>(dimY - 1) || z == 0 || z == static_cast<usize>(dimZ - 1))
            {
              boundaryRoots[root] = true;
            }
          },
          shouldCancel);

      // --- Phase 4b: Replay CCL again to fill interior holes ----------------
      // A third replay of the same CCL (same condition, same union-find) to
      // apply the fill. For each bad voxel whose root is NOT boundary-touching,
      // it must be an interior hole fully enclosed by the sample -- set it to
      // true. Boundary-touching components are exterior and left as-is.
      replayForwardCCL<T>(
          goodVoxels, dimX, dimY, dimZ, holeCCL.unionFind, holeCondition,
          [&boundaryRoots](AbstractDataStore<T>& s, usize idx, int64 root, usize /*x*/, usize /*y*/, usize /*z*/) {
            if(!boundaryRoots[root])
            {
              s.setValue(idx, static_cast<T>(true));
            }
          },
          shouldCancel);
      goodVoxels.flush();
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
IdentifySampleCCL::IdentifySampleCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySampleCCL::~IdentifySampleCCL() noexcept = default;

// -----------------------------------------------------------------------------
Result<> IdentifySampleCCL::operator()()
{
  auto* inputData = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometryPath);

  if(m_InputValues->SliceBySlice)
  {
    ExecuteDataFunction(IdentifySampleSliceBySliceFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles,
                        static_cast<IdentifySampleSliceBySliceFunctor::Plane>(m_InputValues->SliceBySlicePlaneIndex), m_MessageHandler, m_ShouldCancel);
  }
  else
  {
    ExecuteDataFunction(IdentifySampleCCLFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
