#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace nx::core;

namespace
{
template <typename T>
void FillBadDataUpdateTuples(const Int32AbstractDataStore& featureIds, AbstractDataStore<T>& outputDataStore, const std::vector<int32>& neighbors)
{
  usize start = 0;
  usize stop = outputDataStore.getNumberOfTuples();
  const usize numComponents = outputDataStore.getNumberOfComponents();
  for(usize tupleIndex = start; tupleIndex < stop; tupleIndex++)
  {
    const int32 featureName = featureIds[tupleIndex];
    const int32 neighbor = neighbors[tupleIndex];
    if(neighbor == tupleIndex)
    {
      continue;
    }

    if(featureName < 0 && neighbor != -1 && featureIds[static_cast<size_t>(neighbor)] > 0)
    {
      for(usize i = 0; i < numComponents; i++)
      {
        auto value = outputDataStore[neighbor * numComponents + i];
        outputDataStore[tupleIndex * numComponents + i] = value;
      }
    }
  }
}

struct FillBadDataUpdateTuplesFunctor
{
  template <typename T>
  void operator()(const Int32AbstractDataStore& featureIds, IDataArray* outputIDataArray, const std::vector<int32>& neighbors)
  {
    auto& outputStore = outputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    FillBadDataUpdateTuples(featureIds, outputStore, neighbors);
  }
};
} // namespace

// -----------------------------------------------------------------------------
// ChunkAwareUnionFind Implementation
// -----------------------------------------------------------------------------

int64_t ChunkAwareUnionFind::find(int64_t x)
{
  // Create parent entry if it doesn't exist
  if(m_Parent.find(x) == m_Parent.end())
  {
    m_Parent[x] = x;
    m_Rank[x] = 0;
    m_Size[x] = 0;
  }

  // Find root iteratively
  int64_t root = x;
  while(m_Parent[root] != root)
  {
    root = m_Parent[root];
  }

  // Path compression - make all nodes on path point directly to root
  int64_t current = x;
  while(current != root)
  {
    int64_t next = m_Parent[current];
    m_Parent[current] = root;
    current = next;
  }

  return root;
}

void ChunkAwareUnionFind::unite(int64_t a, int64_t b)
{
  int64_t rootA = find(a);
  int64_t rootB = find(b);

  if(rootA == rootB)
  {
    return;
  }

  // Union by rank
  if(m_Rank[rootA] < m_Rank[rootB])
  {
    m_Parent[rootA] = rootB;
  }
  else if(m_Rank[rootA] > m_Rank[rootB])
  {
    m_Parent[rootB] = rootA;
  }
  else
  {
    m_Parent[rootB] = rootA;
    m_Rank[rootA]++;
  }
}

void ChunkAwareUnionFind::addSize(int64_t label, uint64_t count)
{
  // Add size to the label itself, not the root
  // Sizes will be accumulated to roots during flatten()
  m_Size[label] += count;
}

uint64_t ChunkAwareUnionFind::getSize(int64_t label)
{
  int64_t root = find(label);
  auto it = m_Size.find(root);
  if(it == m_Size.end())
  {
    return 0;
  }
  return it->second;
}

void ChunkAwareUnionFind::flatten()
{
  // First pass: flatten all parents
  std::unordered_map<int64_t, int64_t> finalRoots;
  for(auto& [label, parent] : m_Parent)
  {
    int64_t root = find(label);
    finalRoots[label] = root;
  }

  // Second pass: accumulate sizes to roots
  std::unordered_map<int64_t, uint64_t> rootSizes;
  for(const auto& [label, root] : finalRoots)
  {
    rootSizes[root] += m_Size[label];
  }

  // Update parent map and size map
  m_Parent = finalRoots;
  m_Size = rootSizes;
}

// -----------------------------------------------------------------------------
// FillBadData Implementation
// -----------------------------------------------------------------------------

FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FillBadData::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
std::array<int64_t, 6> FillBadData::getNeighborOffsets(const std::array<int64_t, 3>& dims)
{
  // 6 face-connected neighbors: -Z, -Y, -X, +X, +Y, +Z
  return {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};
}

// -----------------------------------------------------------------------------
bool FillBadData::isValidNeighbor(int32_t neighborIdx, int64_t column, int64_t row, int64_t plane, const std::array<int64_t, 3>& dims)
{
  switch(neighborIdx)
  {
  case 0: // -Z
    return plane > 0;
  case 1: // -Y
    return row > 0;
  case 2: // -X
    return column > 0;
  case 3: // +X
    return column < (dims[0] - 1);
  case 4: // +Y
    return row < (dims[1] - 1);
  case 5: // +Z
    return plane < (dims[2] - 1);
  default:
    return false;
  }
}

// -----------------------------------------------------------------------------
void FillBadData::phaseOneCCL(Int32AbstractDataStore& featureIdsStore, ChunkAwareUnionFind& unionFind, std::unordered_map<usize, int64_t>& provisionalLabels, const std::array<int64_t, 3>& dims)
{
  // const auto neighborOffsets = getNeighborOffsets(dims);
  int64_t nextLabel = -1; // Negative labels for bad data regions

  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // Process each chunk sequentially
  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    // Load the current chunk
    featureIdsStore.loadChunk(chunkIdx);

    // Get chunk bounds
    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    // Process voxels in this chunk using scanline algorithm
    // Note: chunk bounds are INCLUSIVE and in [Z, Y, X] order (slowest to fastest)
    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * dims[0] * dims[1] + y * dims[0] + x;

          // Only process bad data (FeatureId == 0)
          if(featureIdsStore[index] != 0)
          {
            continue;
          }

          // Check already-processed neighbors (scanline order: -Z, -Y, -X)
          std::vector<int64_t> neighborLabels;

          // Check -X neighbor
          if(x > 0)
          {
            const usize neighborIdx = index - 1;
            if(provisionalLabels.find(neighborIdx) != provisionalLabels.end() && featureIdsStore[neighborIdx] == 0)
            {
              neighborLabels.push_back(provisionalLabels[neighborIdx]);
            }
          }

          // Check -Y neighbor
          if(y > 0)
          {
            const usize neighborIdx = index - dims[0];
            if(provisionalLabels.find(neighborIdx) != provisionalLabels.end() && featureIdsStore[neighborIdx] == 0)
            {
              neighborLabels.push_back(provisionalLabels[neighborIdx]);
            }
          }

          // Check -Z neighbor
          if(z > 0)
          {
            const usize neighborIdx = index - dims[0] * dims[1];
            if(provisionalLabels.find(neighborIdx) != provisionalLabels.end() && featureIdsStore[neighborIdx] == 0)
            {
              neighborLabels.push_back(provisionalLabels[neighborIdx]);
            }
          }

          int64_t assignedLabel;
          if(neighborLabels.empty())
          {
            // No labeled neighbors, assign new label
            assignedLabel = nextLabel--;
            unionFind.find(assignedLabel); // Initialize in union-find
          }
          else
          {
            // Use the first neighbor's label
            assignedLabel = neighborLabels[0];

            // Track equivalences with other neighbors
            for(size_t i = 1; i < neighborLabels.size(); i++)
            {
              if(neighborLabels[i] != assignedLabel)
              {
                unionFind.unite(assignedLabel, neighborLabels[i]);
              }
            }
          }

          provisionalLabels[index] = assignedLabel;
          unionFind.addSize(assignedLabel, 1);
        }
      }
    }
  }

  // Flush to ensure all chunks are written
  featureIdsStore.flush();
}

// -----------------------------------------------------------------------------
void FillBadData::phaseTwoGlobalResolution(ChunkAwareUnionFind& unionFind, std::unordered_set<int64_t>& smallRegions)
{
  // Flatten the union-find structure to accumulate sizes
  unionFind.flatten();
}

// -----------------------------------------------------------------------------
void FillBadData::phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, const std::unordered_map<usize, int64_t>& provisionalLabels,
                                       const std::unordered_set<int64_t>& smallRegions, ChunkAwareUnionFind& unionFind, size_t maxPhase) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();
  const size_t totalPoints = featureIdsStore.getNumberOfTuples();
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // Collect all unique root labels and their sizes
  std::unordered_map<int64_t, uint64_t> rootSizes;
  for(const auto& [index, label] : provisionalLabels)
  {
    int64_t root = unionFind.find(label);
    if(rootSizes.find(root) == rootSizes.end())
    {
      rootSizes[root] = unionFind.getSize(root);
    }
  }

  // Classify regions as small or large
  std::unordered_set<int64_t> localSmallRegions;
  for(const auto& [root, size] : rootSizes)
  {
    if(static_cast<int32_t>(size) < m_InputValues->minAllowedDefectSizeValue)
    {
      localSmallRegions.insert(root);
    }
  }

  // Process each chunk to relabel voxels
  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStore.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    // Note: chunk bounds are INCLUSIVE and in [Z, Y, X] order (slowest to fastest)
    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * udims[0] * udims[1] + y * udims[0] + x;

          auto labelIter = provisionalLabels.find(index);
          if(labelIter != provisionalLabels.end())
          {
            int64_t root = unionFind.find(labelIter->second);

            if(localSmallRegions.find(root) != localSmallRegions.end())
            {
              // Small region - mark for filling
              featureIdsStore[index] = -1;
            }
            else
            {
              // Large region - keep as 0 or assign new phase
              featureIdsStore[index] = 0;
              if(m_InputValues->storeAsNewPhase && cellPhasesPtr != nullptr)
              {
                (*cellPhasesPtr)[index] = static_cast<int32>(maxPhase) + 1;
              }
            }
          }
        }
      }
    }
  }

  featureIdsStore.flush();
}

// -----------------------------------------------------------------------------
void FillBadData::phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64_t, 3>& dims, size_t numFeatures) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const size_t totalPoints = featureIdsStore.getNumberOfTuples();
  const auto neighborOffsets = getNeighborOffsets(dims);

  std::vector<int32> neighbors(totalPoints, -1);
  std::vector<int32_t> featureNumber(numFeatures + 1, 0);

  // Get list of cell arrays to update (once, before iterations)
  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> voxelArrayNames;
  if(allChildArrays.has_value())
  {
    voxelArrayNames = allChildArrays.value();
  }

  // Create message helper for progress updates (500ms throttle for more frequent updates)
  MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
  auto throttledMessenger = messageHelper.createThrottledMessenger(std::chrono::milliseconds(1000));

  size_t count = 1;
  size_t iteration = 0;

  // Iteratively fill until no -1 voxels remain
  while(count != 0)
  {
    iteration++;
    count = 0;

    // Process all voxels
    for(size_t i = 0; i < totalPoints; i++)
    {
      int32 featureName = featureIdsStore[i];
      if(featureName < 0)
      {
        count++;
        int32 most = 0;

        // Compute position
        auto column = static_cast<int64_t>(i % dims[0]);
        auto row = static_cast<int64_t>((i / dims[0]) % dims[1]);
        auto plane = static_cast<int64_t>(i / (dims[0] * dims[1]));

        // Find most common positive neighbor
        for(int32_t j = 0; j < 6; j++)
        {
          if(!isValidNeighbor(j, column, row, plane, dims))
          {
            continue;
          }

          auto neighborPoint = static_cast<int64_t>(i) + neighborOffsets[j];
          int32 feature = featureIdsStore[neighborPoint];

          if(feature > 0)
          {
            featureNumber[feature]++;
            int32 current = featureNumber[feature];
            if(current > most)
            {
              most = current;
              neighbors[i] = static_cast<int32>(neighborPoint);
            }
          }
        }

        // Reset feature counts
        for(int32_t j = 0; j < 6; j++)
        {
          if(!isValidNeighbor(j, column, row, plane, dims))
          {
            continue;
          }

          int64 neighborPoint = static_cast<int64>(i) + neighborOffsets[j];
          int32 feature = featureIdsStore[neighborPoint];

          if(feature > 0)
          {
            featureNumber[feature] = 0;
          }
        }
      }
    }

    // Update all cell arrays based on neighbors
    for(const auto& cellArrayPath : voxelArrayNames)
    {
      if(cellArrayPath == m_InputValues->featureIdsArrayPath)
      {
        continue;
      }
      auto* oldCellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);

      ExecuteDataFunction(FillBadDataUpdateTuplesFunctor{}, oldCellArray->getDataType(), featureIdsStore, oldCellArray, neighbors);
    }

    // Update FeatureIds array last
    FillBadDataUpdateTuples<int32>(featureIdsStore, featureIdsStore, neighbors);

    // Send progress update
    throttledMessenger.sendThrottledMessage([iteration, count]() { return fmt::format("  Iteration {}: {} voxels remaining to fill", iteration, count); });
  }

  // Send final summary
  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("  Completed in {} iteration{}", iteration, iteration == 1 ? "" : "s")});
}

// -----------------------------------------------------------------------------
Result<> FillBadData::operator()() const
{
  // Get feature IDs array and image geometry
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath)->getDataStoreRef();
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  const size_t totalPoints = featureIdsStore.getNumberOfTuples();

  // Get cell phases array if needed
  Int32Array* cellPhasesPtr = nullptr;
  size_t maxPhase = 0;

  if(m_InputValues->storeAsNewPhase)
  {
    cellPhasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
    for(size_t i = 0; i < totalPoints; i++)
    {
      if((*cellPhasesPtr)[i] > maxPhase)
      {
        maxPhase = (*cellPhasesPtr)[i];
      }
    }
  }

  // Count number of features
  size_t numFeatures = 0;
  for(size_t i = 0; i < totalPoints; i++)
  {
    int32 featureName = featureIdsStore[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  // Data structures for chunk-aware CCL
  ChunkAwareUnionFind unionFind;
  std::unordered_map<usize, int64_t> provisionalLabels;
  std::unordered_set<int64_t> smallRegions;

  // Phase 1: Chunk-Sequential Connected Component Labeling
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 1/4: Labeling connected components..."});
  phaseOneCCL(featureIdsStore, unionFind, provisionalLabels, dims);

  // Phase 2: Global Resolution of equivalences
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 2/4: Resolving region equivalences..."});
  phaseTwoGlobalResolution(unionFind, smallRegions);

  // Phase 3: Relabeling based on region size classification
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 3/4: Classifying region sizes..."});
  phaseThreeRelabeling(featureIdsStore, cellPhasesPtr, provisionalLabels, smallRegions, unionFind, maxPhase);

  // Phase 4: Iterative morphological fill
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 4/4: Filling small defects..."});
  phaseFourIterativeFill(featureIdsStore, dims, numFeatures);

  return {};
}
