#include "IdentifySampleCCL.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
// =============================================================================
// runForwardCCL
// =============================================================================
// Generic Z-slice-sequential Connected Component Labeling function that works
// on any boolean condition. It processes the volume one Z-slice at a time using
// copyIntoBuffer for OOC-friendly reads, and a rolling 2-slice label buffer
// instead of storing labels for the entire volume.
//
// How it works:
//   - Scans voxels in Z-slice order (z, y, x innermost). For each voxel where
//     `condition(sliceData, inSlice)` returns true, checks three backward
//     neighbors (x-1, y-1, z-1) for existing labels.
//   - If no labeled neighbor exists, assigns a new provisional label.
//   - If multiple differently-labeled neighbors exist, unites them in the
//     union-find structure.
//   - Tracks per-label voxel counts (labelSizes) so the largest root can be
//     identified after flattening, without a separate counting pass.
//
// The `condition` lambda determines which voxels to label. For example:
//   - `data[inSlice] == true` labels good voxels (sample identification)
//   - `!data[inSlice]` labels bad voxels (hole detection)
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

  auto sliceData = std::make_unique<T[]>(sliceSize);

  for(int64 z = 0; z < dimZ; z++)
  {
    if(shouldCancel)
    {
      return result;
    }
    store.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<T>(sliceData.get(), sliceSize));

    const usize curOff = (static_cast<usize>(z) % 2) * sliceSize;
    std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
    const usize prevOff = ((static_cast<usize>(z) + 1) % 2) * sliceSize;

    for(int64 y = 0; y < dimY; y++)
    {
      for(int64 x = 0; x < dimX; x++)
      {
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(dimX) + static_cast<usize>(x);

        if(!condition(sliceData.get(), inSlice))
        {
          continue;
        }

        // Backward neighbor checks from label buffer
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

        int64 assignedLabel = 0;
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
// (same Z-slice order, same scanline traversal, same union-find). Since CCL
// label assignment is fully deterministic given the same scan order and
// condition, the re-derived provisional labels match the original ones from
// runForwardCCL exactly. The union-find (already flattened) is then used to
// resolve each provisional label to its root.
//
// The `action` lambda is called for each labeled voxel with its resolved root
// label, the slice data buffer, and the voxel's (x, y, z) coordinates. It
// returns true if the slice data was modified, so the slice can be written back
// via copyFromBuffer. This allows per-voxel decisions (e.g., "mask out if
// root != largestRoot", or "fill if root is an interior hole") without ever
// storing labels for the entire volume.
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
  auto sliceData = std::make_unique<T[]>(sliceSize);
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  int64 nextLabel = 1;

  for(int64 z = 0; z < dimZ; z++)
  {
    if(shouldCancel)
    {
      return;
    }
    store.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<T>(sliceData.get(), sliceSize));
    bool modified = false;

    const usize curOff = (static_cast<usize>(z) % 2) * sliceSize;
    std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
    const usize prevOff = ((static_cast<usize>(z) + 1) % 2) * sliceSize;

    for(int64 y = 0; y < dimY; y++)
    {
      for(int64 x = 0; x < dimX; x++)
      {
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(dimX) + static_cast<usize>(x);

        if(!condition(sliceData.get(), inSlice))
        {
          continue;
        }

        // Re-derive label (same logic, no union-find unites needed since already flattened)
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

        int64 assignedLabel = 0;
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
        if(action(sliceData.get(), inSlice, root, static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z)))
        {
          modified = true;
        }
      }
    }

    if(modified)
    {
      store.copyFromBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<const T>(sliceData.get(), sliceSize));
    }
  }
}

// =============================================================================
// IdentifySampleCCLFunctor
// =============================================================================
// Z-slice-sequential scanline CCL implementation for identifying the largest
// connected component of good voxels in a 3D image geometry, then optionally
// filling interior holes. Processes data one Z-slice at a time using
// copyIntoBuffer/copyFromBuffer for OOC-friendly access, using a 2-slice
// rolling buffer (O(slice) memory) instead of O(volume).
//
// The algorithm has up to four phases:
//
// Phase 1: Forward CCL on good voxels
//   Run runForwardCCL with condition = (goodVoxels[inSlice] == true) to
//   discover all connected components and find the largest one by voxel count.
//
// Phase 2: Replay CCL to mask non-sample voxels
//   Run replayForwardCCL with the same good-voxel condition. For each voxel
//   whose resolved root != largestRoot, set goodVoxels to false (removing
//   satellite regions/noise). No O(volume) label storage is needed -- labels
//   are recomputed on the fly.
//
// Phase 3 (if fillHoles): Forward CCL on bad voxels
//   Run runForwardCCL with condition = (!goodVoxels[inSlice]) to discover all
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

    // --- Phase 1: Forward CCL on good voxels ----------------------------------
    // Discover all connected components of good voxels and find the largest one.
    // The condition lambda selects voxels where goodVoxels[inSlice] is true.
    auto goodCondition = [](const T* data, usize inSlice) -> bool { return static_cast<bool>(data[inSlice]); };
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
    const int64 largestRoot = cclResult.largestRoot;
    replayForwardCCL<T>(
        goodVoxels, dimX, dimY, dimZ, cclResult.unionFind, goodCondition,
        [&largestRoot](T* data, usize inSlice, int64 root, usize /*x*/, usize /*y*/, usize /*z*/) -> bool {
          if(root != largestRoot)
          {
            data[inSlice] = static_cast<T>(false);
            return true;
          }
          return false;
        },
        shouldCancel);

    if(shouldCancel)
    {
      return;
    }

    // --- Phase 3: Forward CCL on bad voxels (hole detection) -----------------
    // Only runs if fillHoles is true. Discovers connected components of
    // non-good voxels (the complement of the sample). These include both
    // exterior empty space and interior holes.
    if(fillHoles)
    {
      // Condition selects voxels where goodVoxels[inSlice] is false (bad data)
      auto holeCondition = [](const T* data, usize inSlice) -> bool { return !static_cast<bool>(data[inSlice]); };
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
          [&boundaryRoots, dimX, dimY, dimZ](T* /*data*/, usize /*inSlice*/, int64 root, usize x, usize y, usize z) -> bool {
            if(x == 0 || x == static_cast<usize>(dimX - 1) || y == 0 || y == static_cast<usize>(dimY - 1) || z == 0 || z == static_cast<usize>(dimZ - 1))
            {
              boundaryRoots[root] = true;
            }
            return false; // Never modifies data
          },
          shouldCancel);

      if(shouldCancel)
      {
        return;
      }

      // --- Phase 4b: Replay CCL again to fill interior holes ----------------
      // A third replay of the same CCL (same condition, same union-find) to
      // apply the fill. For each bad voxel whose root is NOT boundary-touching,
      // it must be an interior hole fully enclosed by the sample -- set it to
      // true. Boundary-touching components are exterior and left as-is.
      replayForwardCCL<T>(
          goodVoxels, dimX, dimY, dimZ, holeCCL.unionFind, holeCondition,
          [&boundaryRoots](T* data, usize inSlice, int64 root, usize /*x*/, usize /*y*/, usize /*z*/) -> bool {
            if(!boundaryRoots[root])
            {
              data[inSlice] = static_cast<T>(true);
              return true;
            }
            return false;
          },
          shouldCancel);
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
