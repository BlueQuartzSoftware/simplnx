#include "IdentifySampleCCL.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
// Helper: Run forward CCL on a boolean condition using a 2-slice rolling buffer.
// Returns the VectorUnionFind, rootSizes, and the next label.
// The condition lambda takes (goodVoxels store, index) and returns true for voxels to label.
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

  // Rolling 2-slice buffer: only current + previous Z-slice labels
  std::vector<int64> labelBuffer(2 * sliceSize, 0);
  // Size tracking per label for finding largest component without a rescan
  std::vector<uint64> labelSizes;
  labelSizes.push_back(0); // index 0 unused

  const uint64 numChunks = store.getNumberOfChunks();
  // Track last cleared Z-slice to avoid re-clearing when a Z-slice spans multiple chunks
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

// Helper: Re-derive labels for each voxel using a second forward CCL pass
// with a rolling buffer, then apply an action lambda for each labeled voxel.
// This avoids storing labels for the entire volume.
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

// Chunk-sequential scanline CCL implementation for 3D volumes.
// Processes data in chunk order to avoid random chunk access in OOC mode.
// Uses a 2-slice rolling buffer (O(slice) memory) instead of O(volume).
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

    // Phase 1: Forward CCL on good voxels using rolling buffer
    messageHandler(IFilter::Message::Type::Info, "Identifying sample regions...");
    auto goodCondition = [](const AbstractDataStore<T>& s, usize idx) -> bool { return static_cast<bool>(s[idx]); };
    auto cclResult = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, goodCondition, shouldCancel);

    if(shouldCancel || cclResult.largestRoot < 0)
    {
      return;
    }

    // Phase 2: Mask out non-sample voxels by replaying CCL
    // Re-derive labels using a second forward pass with rolling buffer,
    // then set non-largest-component voxels to false.
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

    // Phase 3: Hole-fill CCL on bad voxels (if fillHoles is true)
    if(fillHoles)
    {
      messageHandler(IFilter::Message::Type::Info, "Filling holes in sample...");

      // Forward CCL on non-good voxels (holes)
      auto holeCondition = [](const AbstractDataStore<T>& s, usize idx) -> bool { return !static_cast<bool>(s[idx]); };
      auto holeCCL = runForwardCCL<T>(goodVoxels, dimX, dimY, dimZ, holeCondition, shouldCancel);

      if(shouldCancel)
      {
        return;
      }

      // Determine which hole roots touch the domain boundary
      // Replay CCL to check boundary status without storing full labels
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

      // Phase 4: Fill interior holes by replaying CCL once more
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
