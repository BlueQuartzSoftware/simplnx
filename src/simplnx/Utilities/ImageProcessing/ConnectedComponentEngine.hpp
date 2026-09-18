#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp" // CreateDataStoreWithFormat (fallback uint32 provisional scratch)
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
template <class T>
Result<> ValidateConnectedComponentVolume(const AbstractDataStore<T>& inStore, const SizeVec3& dims, usize& sliceValues, usize& volumeValues)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    sliceValues = 0;
    volumeValues = 0;
    return {};
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max || dims[2] > k_Int64Max)
  {
    return MakeErrorResult(-8370, fmt::format("Connected-component image dimensions exceed the supported signed index range. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  auto checkedMultiply = [](usize left, usize right, usize& product) {
    if(left != 0 && right > std::numeric_limits<usize>::max() / left)
    {
      return false;
    }
    product = left * right;
    return true;
  };
  if(!checkedMultiply(dims[0], dims[1], sliceValues) || !checkedMultiply(sliceValues, dims[2], volumeValues))
  {
    return MakeErrorResult(-8371, fmt::format("Connected-component image dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(inStore.getSize() != volumeValues)
  {
    return MakeErrorResult(-8372, fmt::format("Connected-component input store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", inStore.getSize(), volumeValues,
                                              dims[0], dims[1], dims[2]));
  }
  return {};
}

// A maximal foreground run in one x-scanline: x in [start, start+length-1], provisional label.
struct CcRun
{
  int64 start;
  int64 length;
  uint32 label;
};
using CcLine = std::vector<CcRun>;

inline constexpr usize k_ConnectedComponent2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_ConnectedComponent2DMetadataHeadroomBytes = 64ULL * 1024ULL;

struct ConnectedComponent2DPlan
{
  bool useTiles = false;
  usize coreRows = 0;
  usize coreColumns = 0;
  usize maximumRunsPerLine = 0;
  usize residentBytes = 0;
};

inline bool ConnectedComponentCheckedAdd(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool ConnectedComponentCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

struct ConnectedComponentResidentProvisionalAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

inline bool ShouldUseConnectedComponentResidentProvisional(const SizeVec3& dims)
{
  return dims[2] > 1;
}

inline Result<usize> CalculateConnectedComponentResidentProvisionalBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize requiredBytes = 0;
  if(!ConnectedComponentCheckedMultiply(dims[0], dims[1], sliceValues) || !ConnectedComponentCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !ConnectedComponentCheckedMultiply(volumeValues, sizeof(uint32), requiredBytes))
  {
    return MakeErrorResult<usize>(-8764, fmt::format("Connected-component dimensions {} x {} x {} overflow while sizing the resident uint32 provisional-label volume.", dims[0], dims[1], dims[2]));
  }
  return {requiredBytes};
}

inline Result<ConnectedComponentResidentProvisionalAllocation> ReserveConnectedComponentResidentProvisional(const SizeVec3& dims)
{
  auto requiredResult = CalculateConnectedComponentResidentProvisionalBytes(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<ConnectedComponentResidentProvisionalAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {ConnectedComponentResidentProvisionalAllocation{std::move(reservation), requiredResult.value()}};
}

struct ConnectedComponentResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

template <class T>
Result<usize> CalculateConnectedComponentResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  const usize maximumRunsPerLine = dims[0] / 2 + dims[0] % 2;
  usize sliceValues = 0;
  usize volumeValues = 0;
  usize maximumRunsPerPlane = 0;
  usize maximumRuns = 0;
  usize labelEntries = 0;
  usize imageBytesPerValue = 0;
  usize imageBytes = 0;
  usize tableBytes = 0;
  usize planeBytesPerValue = 0;
  usize planeBytes = 0;
  usize lineRecords = 0;
  usize lineBytes = 0;
  usize requiredBytes = 0;
  constexpr usize k_TableBytesPerLabel = 2 * sizeof(uint32) + 2 * sizeof(uint64) + 2 * sizeof(uint32);
  if(!ConnectedComponentCheckedMultiply(dims[0], dims[1], sliceValues) || !ConnectedComponentCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !ConnectedComponentCheckedMultiply(maximumRunsPerLine, dims[1], maximumRunsPerPlane) || !ConnectedComponentCheckedMultiply(maximumRunsPerPlane, dims[2], maximumRuns) ||
     !ConnectedComponentCheckedAdd(maximumRuns, usize{1}, labelEntries) || !ConnectedComponentCheckedAdd(sizeof(T), 2 * sizeof(uint32), imageBytesPerValue) ||
     !ConnectedComponentCheckedMultiply(volumeValues, imageBytesPerValue, imageBytes) || !ConnectedComponentCheckedMultiply(labelEntries, k_TableBytesPerLabel, tableBytes) ||
     !ConnectedComponentCheckedAdd(sizeof(T), sizeof(uint32), planeBytesPerValue))
  {
    return MakeErrorResult<usize>(
        -8765, fmt::format("Connected-component dimensions {} x {} x {} and {}-byte input values overflow while sizing the full resident working state.", dims[0], dims[1], dims[2], sizeof(T)));
  }

  planeBytesPerValue = std::max(planeBytesPerValue, 2 * sizeof(uint32));
  if(!ConnectedComponentCheckedMultiply(sliceValues, planeBytesPerValue, planeBytes) || !ConnectedComponentCheckedMultiply(maximumRunsPerPlane, usize{4}, lineRecords) ||
     !ConnectedComponentCheckedMultiply(lineRecords, sizeof(CcRun), lineBytes) || !ConnectedComponentCheckedAdd(imageBytes, tableBytes, requiredBytes) ||
     !ConnectedComponentCheckedAdd(requiredBytes, planeBytes, requiredBytes) || !ConnectedComponentCheckedAdd(requiredBytes, lineBytes, requiredBytes))
  {
    return MakeErrorResult<usize>(
        -8765, fmt::format("Connected-component dimensions {} x {} x {} and {}-byte input values overflow while sizing the full resident working state.", dims[0], dims[1], dims[2], sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<ConnectedComponentResidentMemoryAllocation> ReserveConnectedComponentResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateConnectedComponentResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<ConnectedComponentResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {ConnectedComponentResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

struct ConnectedComponentProvisionalSelection
{
  // Declaration order is intentional: destruction runs in reverse, so the datastore releases its memory before
  // the reservation returns that headroom to the shared cache budget.
  std::optional<ConnectedComponentResidentProvisionalAllocation> residentAllocation;
  std::shared_ptr<AbstractDataStore<uint32>> dataStore;

  [[nodiscard]] bool holdsResidentState() const noexcept
  {
    return dataStore != nullptr && residentAllocation.has_value() && residentAllocation->holdsCompleteState();
  }
};

inline Result<ConnectedComponentProvisionalSelection> TryCreateConnectedComponentResidentProvisionalSelection(const SizeVec3& dims, bool usesOutOfCoreEndpoint)
{
  ConnectedComponentProvisionalSelection selection;
  if(usesOutOfCoreEndpoint && ShouldUseConnectedComponentResidentProvisional(dims))
  {
    auto allocationResult = ReserveConnectedComponentResidentProvisional(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<ConnectedComponentProvisionalSelection>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        auto residentDataStore = std::make_shared<DataStore<uint32>>(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
        selection.residentAllocation.emplace(std::move(allocation));
        selection.dataStore = std::move(residentDataStore);
        return {std::move(selection)};
      } catch(const std::bad_alloc&)
      {
        // The allocation releases the complete-state reservation before the fallback route starts.
      }
    }
  }

  return {std::move(selection)};
}

inline Result<ConnectedComponentProvisionalSelection> CreateConnectedComponentProvisionalSelection(const SizeVec3& dims, bool usesOutOfCoreEndpoint, const std::string& dataFormat)
{
  auto residentResult = TryCreateConnectedComponentResidentProvisionalSelection(dims, usesOutOfCoreEndpoint);
  if(residentResult.invalid())
  {
    return ConvertInvalidResult<ConnectedComponentProvisionalSelection>(std::move(residentResult));
  }
  auto selection = std::move(residentResult.value());
  if(selection.holdsResidentState())
  {
    return {std::move(selection)};
  }

  selection.dataStore = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(dataFormat, ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1});
  return {std::move(selection)};
}

template <class T>
std::string SelectConnectedComponentProvisionalDataFormat(const AbstractDataStore<T>& inStore, const AbstractDataStore<uint32>& outStore)
{
  if(inStore.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    return inStore.getDataFormat();
  }
  if(outStore.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    return outStore.getDataFormat();
  }
  return inStore.getDataFormat();
}

template <class T>
Result<ConnectedComponent2DPlan> CreateConnectedComponent2DPlan(usize dimX, usize dimY, bool writesProvisional, usize targetBytes = k_ConnectedComponent2DTargetBytes)
{
  if(dimX == 0 || dimY == 0 || targetBytes == 0)
  {
    return MakeErrorResult<ConnectedComponent2DPlan>(
        -8374, fmt::format("Connected-component true-2-D plan requires nonzero dimensions and target bytes. Dimensions: {} x {}; target: {} bytes.", dimX, dimY, targetBytes));
  }

  ConnectedComponent2DPlan plan;
  plan.maximumRunsPerLine = dimX / 2 + dimX % 2;

  usize oneLineCapacityBytes = 0;
  usize bothLineCapacityBytes = 0;
  usize fullWidthFixedBytes = 0;
  usize rowValueBytes = 0;
  const usize fullWidthObjectBytes = k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<T>) + 2 * sizeof(CcLine) + (writesProvisional ? sizeof(std::vector<uint32>) : 0);
  const usize valuesPerPixel = sizeof(T) + (writesProvisional ? sizeof(uint32) : 0);
  if(!ConnectedComponentCheckedMultiply(plan.maximumRunsPerLine, sizeof(CcRun), oneLineCapacityBytes) || !ConnectedComponentCheckedMultiply(oneLineCapacityBytes, 2, bothLineCapacityBytes) ||
     !ConnectedComponentCheckedAdd(fullWidthObjectBytes, bothLineCapacityBytes, fullWidthFixedBytes) || !ConnectedComponentCheckedMultiply(dimX, valuesPerPixel, rowValueBytes))
  {
    return MakeErrorResult<ConnectedComponent2DPlan>(-8374,
                                                     fmt::format("Connected-component true-2-D full-width plan overflowed for dimensions {} x {} and {}-byte input values.", dimX, dimY, sizeof(T)));
  }

  if(fullWidthFixedBytes <= targetBytes && rowValueBytes <= targetBytes - fullWidthFixedBytes)
  {
    plan.coreRows = std::min(dimY, (targetBytes - fullWidthFixedBytes) / rowValueBytes);
    plan.coreColumns = dimX;
    usize blockBytes = 0;
    if(plan.coreRows == 0 || !ConnectedComponentCheckedMultiply(plan.coreRows, rowValueBytes, blockBytes) || !ConnectedComponentCheckedAdd(fullWidthFixedBytes, blockBytes, plan.residentBytes))
    {
      return MakeErrorResult<ConnectedComponent2DPlan>(-8374,
                                                       fmt::format("Connected-component true-2-D full-width plan could not fit one row for dimensions {} x {} in {} bytes.", dimX, dimY, targetBytes));
    }
    return {plan};
  }

  plan.useTiles = true;
  plan.coreRows = 1;
  const usize tileFixedBytes = k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<T>) + 2 * sizeof(std::vector<uint32>) + 2 * sizeof(uint32);
  const usize bytesPerColumn = sizeof(T) + 2 * sizeof(uint32);
  if(tileFixedBytes > targetBytes || bytesPerColumn > targetBytes - tileFixedBytes)
  {
    return MakeErrorResult<ConnectedComponent2DPlan>(
        -8374, fmt::format("Connected-component true-2-D tiled plan cannot fit one input/provisional column and previous-row halos for dimensions {} x {} in {} bytes.", dimX, dimY, targetBytes));
  }
  plan.coreColumns = std::min(dimX, (targetBytes - tileFixedBytes) / bytesPerColumn);
  usize tileColumnBytes = 0;
  if(plan.coreColumns == 0 || !ConnectedComponentCheckedMultiply(plan.coreColumns, bytesPerColumn, tileColumnBytes) ||
     !ConnectedComponentCheckedAdd(tileFixedBytes, tileColumnBytes, plan.residentBytes))
  {
    return MakeErrorResult<ConnectedComponent2DPlan>(-8374, fmt::format("Connected-component true-2-D tiled plan overflowed for dimensions {} x {} and {}-byte input values.", dimX, dimY, sizeof(T)));
  }
  return {plan};
}

inline usize AlignConnectedComponent2DRows(usize maximumRows, usize dimX, const std::optional<ShapeType>& firstChunkShape, const std::optional<ShapeType>& secondChunkShape)
{
  if(maximumRows == 0)
  {
    return 0;
  }
  usize rowAlignment = 1;
  bool alignmentOverflow = false;
  const auto includeChunkShape = [dimX, &rowAlignment, &alignmentOverflow](const std::optional<ShapeType>& chunkShape) {
    if(alignmentOverflow)
    {
      return;
    }
    if(!chunkShape.has_value() || chunkShape->size() < 3 || (*chunkShape)[0] != 1 || (*chunkShape)[1] == 0 || (*chunkShape)[2] != dimX)
    {
      return;
    }
    const usize chunkRows = (*chunkShape)[1];
    const usize divisor = std::gcd(rowAlignment, chunkRows);
    if(rowAlignment > std::numeric_limits<usize>::max() / (chunkRows / divisor))
    {
      alignmentOverflow = true;
      return;
    }
    rowAlignment *= chunkRows / divisor;
  };
  includeChunkShape(firstChunkShape);
  includeChunkShape(secondChunkShape);
  if(alignmentOverflow || rowAlignment <= 1 || rowAlignment > maximumRows)
  {
    return maximumRows;
  }
  const usize alignedRows = (maximumRows / rowAlignment) * rowAlignment;
  return alignedRows == 0 ? maximumRows : alignedRows;
}

inline Result<usize> CreateConnectedComponent2DFinalChunkValues(usize totalValues, usize dimX, usize targetBytes, const std::optional<ShapeType>& provisionalChunkShape,
                                                                const std::optional<ShapeType>& outputChunkShape)
{
  const usize fixedBytes = k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<uint32>);
  if(totalValues == 0 || dimX == 0 || targetBytes <= fixedBytes || (targetBytes - fixedBytes) / sizeof(uint32) == 0)
  {
    return MakeErrorResult<usize>(
        -8374, fmt::format("Connected-component true-2-D finalization cannot fit one uint32 label value for {} values, row width {}, and target {} bytes.", totalValues, dimX, targetBytes));
  }
  usize chunkValues = std::min(totalValues, (targetBytes - fixedBytes) / sizeof(uint32));
  if(chunkValues >= dimX)
  {
    const usize maximumRows = chunkValues / dimX;
    const usize alignedRows = AlignConnectedComponent2DRows(maximumRows, dimX, provisionalChunkShape, outputChunkShape);
    chunkValues = alignedRows * dimX;
  }
  return {chunkValues};
}

class ConnectedComponentProvisionalStore
{
public:
  explicit ConnectedComponentProvisionalStore(AbstractDataStore<uint32>& dataStore)
  : m_DataStore(&dataStore)
  {
  }

  explicit ConnectedComponentProvisionalStore(SweepTemporaryStore<uint32>& fixedRecordStore)
  : m_FixedRecordStore(&fixedRecordStore)
  {
  }

  Result<> copyIntoBuffer(usize valueOffset, nonstd::span<uint32> values) const
  {
    return m_DataStore != nullptr ? m_DataStore->copyIntoBuffer(valueOffset, values) : m_FixedRecordStore->copyIntoBuffer(valueOffset, values);
  }

  Result<> copyFromBuffer(usize valueOffset, nonstd::span<const uint32> values)
  {
    return m_DataStore != nullptr ? m_DataStore->copyFromBuffer(valueOffset, values) : m_FixedRecordStore->copyFromBuffer(valueOffset, values);
  }

  std::optional<ShapeType> getChunkShape() const
  {
    return m_DataStore != nullptr ? m_DataStore->getChunkShape() : std::nullopt;
  }

private:
  AbstractDataStore<uint32>* m_DataStore = nullptr;
  SweepTemporaryStore<uint32>* m_FixedRecordStore = nullptr;
};

// Union-find over provisional labels (1-based; index 0 unused). union-by-smaller-root, matching ITK ScanlineFilterCommon.
struct CcUnionFind
{
  std::vector<uint32> parent{0u}; // parent[0] unused; grows as labels are created
  uint32 makeLabel()              // create a new provisional label = its own root
  {
    const uint32 l = static_cast<uint32>(parent.size());
    parent.push_back(l);
    return l;
  }
  uint32 find(uint32 l) const noexcept // LookupSet: follow parents to the root
  {
    while(l != parent[l])
    {
      l = parent[l];
    }
    return l;
  }
  void link(uint32 a, uint32 b) // LinkLabels: union by smaller root
  {
    const uint32 e1 = find(a);
    const uint32 e2 = find(b);
    if(e1 < e2)
    {
      parent[e2] = e1;
    }
    else if(e2 < e1)
    {
      parent[e1] = e2;
    }
  }
  // CreateConsecutive (verbatim ITK): roots (parent[i]==i) get consecutive labels 1,2,... in ascending-i order; bg=0.
  // Returns {consecutive[], numberOfObjects}.
  std::pair<std::vector<uint32>, uint32> createConsecutive() const
  {
    const usize n = parent.size();
    std::vector<uint32> consecutive(n, 0u);
    uint32 consecutiveLabel = 0;
    uint32 count = 0;
    for(usize i = 1; i < n; ++i)
    {
      if(parent[i] == i)
      {
        ++consecutiveLabel; // skips 0 (background)
        consecutive[i] = consecutiveLabel;
        ++count;
      }
    }
    return {consecutive, count};
  }
};

// Encode one gathered x-row into unlabeled foreground runs. `Pred` is a compile-time predicate functor so the
// per-voxel foreground test on the hot path is a direct call.
template <class T, class Pred>
inline void EncodeLineGeometryInto(nonstd::span<const T> row, int64 nX, const Pred& pred, CcLine& line)
{
  line.clear();
  int64 x = 0;
  while(x < nX)
  {
    if(pred(row[static_cast<usize>(x)]))
    {
      const int64 start = x;
      while(x < nX && pred(row[static_cast<usize>(x)]))
      {
        ++x;
      }
      line.push_back(CcRun{start, x - start, 0u});
    }
    else
    {
      ++x;
    }
  }
}

// Encode one gathered x-row and assign each run a fresh provisional label.
template <class T, class Pred>
inline void EncodeLineInto(nonstd::span<const T> row, int64 nX, const Pred& pred, CcUnionFind& uf, CcLine& line)
{
  EncodeLineGeometryInto(row, nX, pred, line);
  for(CcRun& run : line)
  {
    run.label = uf.makeLabel();
  }
}

// CompareLines: union overlapping runs between the current line and an already-scanned neighbor line. `offset` = 0 for
// face connectivity, 1 for the fully-connected in-x diagonal (ITK ScanlineFilterCommon::CompareLines: extend the
// neighbor run to [nStart-offset, nLast+offset] and test intersection with the current run [cStart,cLast]).
inline void CompareLines(const CcLine& current, const CcLine& neighbor, int64 offset, CcUnionFind& uf)
{
  usize neighborIndex = 0;
  for(const CcRun& c : current)
  {
    const int64 cStart = c.start;
    const int64 cLast = c.start + c.length - 1;
    while(neighborIndex < neighbor.size())
    {
      const CcRun& n = neighbor[neighborIndex];
      if(n.start + n.length - 1 + offset >= cStart)
      {
        break;
      }
      ++neighborIndex;
    }
    for(usize index = neighborIndex; index < neighbor.size(); ++index)
    {
      const CcRun& n = neighbor[index];
      const int64 ss1 = n.start - offset;
      if(ss1 > cLast)
      {
        break;
      }
      const int64 ee2 = n.start + n.length - 1 + offset;
      if(ss1 <= cLast && ee2 >= cStart) // extended neighbor intersects current run
      {
        uf.link(c.label, n.label);
      }
    }
  }
}

/**
 * @brief Streaming three-dimensional pass 1 (scanline union-find): read the input in a bounded, working-memory-derived
 * band of Z planes per bulk transfer (never a single fixed plane count; degrades to one plane under a tight grant),
 * encode independent x-rows in parallel per plane, assign exact raster-order provisional labels, union prior neighbor
 * lines, and optionally write a provisional-label plane. If requested, the pass tallies each run length by its
 * provisional label. CountConnectedComponents resolves these totals after union operations finish.
 *
 * Neighbor-line set transcribed from ITK's `itkConnectedComponentAlgorithm.h::setConnectivityPrevious` (called by
 * `ScanlineFilterCommon::SetupLineOffsets` with `wholeNeighborhood=false`):
 *  - face (!fullyConnected): previous row same z-plane (y-1,z) and previous z-plane same row (y,z-1), offset=0.
 *  - full (fullyConnected): setConnectivityPrevious activates exactly 4 offsets in (dy,dz) -- (-1,-1),(0,-1),(1,-1),
 *    (-1,0) -- i.e. curPlane[y-1] (dy=-1,dz=0), prevPlane[y] (dy=0,dz=-1), and the two previous-plane diagonal rows
 *    prevPlane[y-1] (dy=-1,dz=-1) and prevPlane[y+1] (dy=1,dz=-1). Critically, ITK's `ScanlineFilterCommon::CompareLines`
 *    computes `offset = (m_FullyConnected || sameLine) ? 1 : 0`, and `ComputeEquivalence` always calls `CompareLines`
 *    with `sameLineOffset=false` (so `sameLine` is always false there) -- meaning offset is driven ENTIRELY by
 *    `m_FullyConnected`, uniformly across all 4 neighbor-line comparisons, not just the diagonal ones. So under full
 *    connectivity ALL FOUR neighbor-line comparisons (including the two face-aligned ones) use offset=1.
 * @tparam T Input value type.
 * @tparam Pred Foreground predicate type.
 * @param inStore Input image store.
 * @param provisional Optional provisional-label store.
 * @param dims Image dimensions in X, Y, Z order.
 * @param pred Predicate that identifies foreground values.
 * @param fullyConnected True to use full connectivity.
 * @param shouldCancel Shared cancellation flag.
 * @param uf Union-find state that receives provisional-label links.
 * @param sizeByLabel Optional per-provisional-label size totals.
 * @return A valid result or an input or provisional-store transfer error.
 */
template <class T, class Pred>
Result<> Pass1ThreeDimensional(const AbstractDataStore<T>& inStore, ConnectedComponentProvisionalStore* provisional, const SizeVec3& dims, const Pred& pred, bool fullyConnected,
                               const std::atomic_bool& shouldCancel, CcUnionFind& uf, std::vector<uint64>* sizeByLabel)
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  if(nX == 0 || nY == 0 || nZ == 0)
  {
    return {};
  }
  const usize nYu = static_cast<usize>(nY);
  const usize nZu = static_cast<usize>(nZ);
  const usize sliceValues = static_cast<usize>(nX) * nYu;

  // Size a bounded read-ahead band so each input read presents multiple whole Z-plane chunks instead of exactly
  // one (a lone Z-plane is exactly one store chunk for this engine's typical geometry, and
  // ParallelForChunkPositions runs its codec fully serially whenever a call presents one chunk or fewer). This
  // widens only the READ granularity below; the per-plane line encoding, union-find, and optional
  // provisional-label write are unchanged. Mirrors ReplayThreeDimensionalLabels's grouping (a modest 1/20
  // fraction of the full-volume byte count, floored at one plane) and degrades to one plane -- reproducing the
  // previous unconditional per-plane read exactly -- whenever the grant cannot hold even one extra plane.
  usize planeBytes = 0;
  usize fullVolumeBytes = 0;
  if(!ConnectedComponentCheckedMultiply(sliceValues, sizeof(T), planeBytes) || !ConnectedComponentCheckedMultiply(planeBytes, nZu, fullVolumeBytes))
  {
    return MakeErrorResult(-8769, fmt::format("Connected-component pass 1 dimensions {} x {} x {} and {}-byte input values overflow while sizing the input read-ahead band.", static_cast<usize>(nX),
                                              nYu, nZu, sizeof(T)));
  }
  const uint64 usefulBandBytes = ResolveWorkingMemoryFractionBytes(fullVolumeBytes, 1, 20, planeBytes);
  auto bandReservation = ReserveWorkingMemory(usefulBandBytes, usefulBandBytes);
  usize planesPerBand = 1;
  if(bandReservation.sizeBytes() >= planeBytes)
  {
    planesPerBand = std::min(nZu, static_cast<usize>(bandReservation.sizeBytes() / planeBytes));
    bandReservation.shrinkTo(planesPerBand * planeBytes);
  }
  else
  {
    bandReservation.shrinkTo(0);
  }

  std::vector<CcLine> prevPlane(nYu); // previous z-plane's line encodings (empty vector = no plane yet)
  std::vector<CcLine> curPlane(nYu);  // current z-plane's line encodings, built up row by row
  std::vector<T> planeBand(planesPerBand * sliceValues);
  std::vector<uint32> provPlane;
  if(provisional != nullptr)
  {
    provPlane.resize(sliceValues);
  }
  ParallelDataAlgorithm parallelRows;
  parallelRows.setRange(0, nYu);

  usize bandFirstZ = 0;       // first Z-plane index currently resident in planeBand
  usize bandLoadedPlanes = 0; // number of planes currently resident in planeBand, starting at bandFirstZ

  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize zu = static_cast<usize>(z);
    const usize planeOffset = zu * sliceValues;
    if(zu >= bandFirstZ + bandLoadedPlanes)
    {
      // Every plane is still read exactly once, in ascending order -- only the read granularity changes from
      // one plane per call to planesPerBand planes per call.
      bandFirstZ = zu;
      bandLoadedPlanes = std::min(planesPerBand, nZu - zu);
      if(Result<> r = inStore.copyIntoBuffer(bandFirstZ * sliceValues, nonstd::span<T>(planeBand.data(), bandLoadedPlanes * sliceValues)); r.invalid())
      {
        return r;
      }
    }
    const T* planeData = planeBand.data() + (zu - bandFirstZ) * sliceValues;
    if(provisional != nullptr)
    {
      std::fill(provPlane.begin(), provPlane.end(), 0u);
    }

    // Each worker writes one CcLine and reads only its row from the resident band.
    const auto encodeRows = [&](const Range& range) {
      const Pred localPredicate = pred;
      for(usize y = range.min(); y < range.max(); ++y)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize rowOffset = y * static_cast<usize>(nX);
        EncodeLineGeometryInto<T, Pred>(nonstd::span<const T>(planeData + rowOffset, static_cast<usize>(nX)), nX, localPredicate, curPlane[y]);
      }
    };
    parallelRows.execute(encodeRows);
    if(shouldCancel)
    {
      return {};
    }

    for(int64 y = 0; y < nY; ++y)
    {
      const usize rowOffset = static_cast<usize>(y) * static_cast<usize>(nX);
      CcLine& line = curPlane[static_cast<usize>(y)];
      for(CcRun& run : line)
      {
        run.label = uf.makeLabel();
      }

      // ---- union against already-scanned neighbor lines ----
      // offset is driven uniformly by fullyConnected (ITK's ComputeEquivalence always passes sameLine=false, so
      // CompareLines's offset = m_FullyConnected ? 1 : 0 for every neighbor-line comparison, not just the diagonals).
      const usize yu = static_cast<usize>(y);
      const int64 offset = fullyConnected ? 1 : 0;
      if(y > 0)
      {
        CompareLines(line, curPlane[yu - 1], offset, uf); // face: previous row, same z-plane
      }
      if(z > 0)
      {
        CompareLines(line, prevPlane[yu], offset, uf); // face: same row, previous z-plane
      }
      if(fullyConnected && z > 0)
      {
        // diagonal previous-plane neighbors (full connectivity only)
        if(yu > 0)
        {
          CompareLines(line, prevPlane[yu - 1], offset, uf); // diagonal: (y-1, z-1)
        }
        if(yu + 1 < nYu)
        {
          CompareLines(line, prevPlane[yu + 1], offset, uf); // diagonal: (y+1, z-1)
        }
      }

      if(sizeByLabel != nullptr)
      {
        for(const CcRun& run : line)
        {
          if(sizeByLabel->size() <= run.label)
          {
            sizeByLabel->resize(static_cast<usize>(run.label) + 1, 0u);
          }
          (*sizeByLabel)[run.label] += static_cast<uint64>(run.length);
        }
      }
      if(provisional != nullptr)
      {
        for(const CcRun& run : line)
        {
          for(int64 x = run.start; x < run.start + run.length; ++x)
          {
            provPlane[rowOffset + static_cast<usize>(x)] = run.label;
          }
        }
      }
    }
    if(provisional != nullptr)
    {
      if(Result<> r = provisional->copyFromBuffer(planeOffset, nonstd::span<const uint32>(provPlane.data(), sliceValues)); r.invalid())
      {
        return r;
      }
    }
    prevPlane.swap(curPlane);
  }
  return {};
}

/**
 * @brief Recreates raster-order run labels and writes the final three-dimensional label image.
 * @tparam T Input value type.
 * @tparam Pred Foreground predicate type.
 * @param inStore Input image store.
 * @param outStore Output label store.
 * @param dims Image dimensions in X, Y, Z order.
 * @param pred Predicate that identifies foreground values.
 * @param finalLabel Final label for each raster-order provisional label.
 * @param shouldCancel Shared cancellation flag.
 * @return A valid result or an input or output transfer error.
 */
template <class T, class Pred>
Result<> ReplayThreeDimensionalLabels(const AbstractDataStore<T>& inStore, AbstractDataStore<uint32>& outStore, const SizeVec3& dims, const Pred& pred, const std::vector<uint32>& finalLabel,
                                      const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceValues = dimX * dimY;
  usize stagingBytesPerPlane = 0;
  usize fullStagingBytes = 0;
  usize rowMetadataBytes = 0;
  constexpr usize k_StagingValuesPerRow = 2;
  if(!ConnectedComponentCheckedMultiply(dimY, k_StagingValuesPerRow * sizeof(usize), rowMetadataBytes) ||
     !ConnectedComponentCheckedMultiply(sliceValues, sizeof(T) + sizeof(uint32), stagingBytesPerPlane) || !ConnectedComponentCheckedAdd(stagingBytesPerPlane, rowMetadataBytes, stagingBytesPerPlane) ||
     !ConnectedComponentCheckedMultiply(stagingBytesPerPlane, dimZ, fullStagingBytes))
  {
    return MakeErrorResult(-8766, fmt::format("Connected-component replay dimensions {} x {} x {} overflow while sizing grouped plane staging.", dimX, dimY, dimZ));
  }

  const uint64 usefulStagingBytes = ResolveWorkingMemoryFractionBytes(fullStagingBytes, 1, 20, stagingBytesPerPlane);
  auto stagingReservation = ReserveWorkingMemory(usefulStagingBytes, usefulStagingBytes);
  usize planesPerGroup = 1;
  if(stagingReservation.sizeBytes() >= stagingBytesPerPlane)
  {
    planesPerGroup = std::min(dimZ, static_cast<usize>(stagingReservation.sizeBytes() / stagingBytesPerPlane));
    stagingReservation.shrinkTo(planesPerGroup * stagingBytesPerPlane);
  }
  else
  {
    stagingReservation.shrinkTo(0);
  }

  const usize maximumGroupValues = planesPerGroup * sliceValues;
  const usize maximumGroupRows = planesPerGroup * dimY;
  std::vector<T> inputGroup(maximumGroupValues);
  std::vector<uint32> outputGroup(maximumGroupValues);
  std::vector<usize> rowRunCounts(maximumGroupRows);
  std::vector<usize> rowLabelOffsets(maximumGroupRows);
  usize provisionalLabel = 0;
  ParallelDataAlgorithm parallelRows;

  for(usize firstPlane = 0; firstPlane < dimZ; firstPlane += planesPerGroup)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize groupPlaneCount = std::min(planesPerGroup, dimZ - firstPlane);
    const usize groupRows = groupPlaneCount * dimY;
    const usize groupValues = groupPlaneCount * sliceValues;
    const usize groupOffset = firstPlane * sliceValues;
    if(Result<> result = inStore.copyIntoBuffer(groupOffset, nonstd::span<T>(inputGroup.data(), groupValues)); result.invalid())
    {
      return result;
    }

    // Row workers access only local plane buffers. Store transfers remain serial.
    parallelRows.setRange(0, groupRows);
    const auto countRowRuns = [&](const Range& range) {
      const Pred localPredicate = pred;
      for(usize rowIndex = range.min(); rowIndex < range.max(); ++rowIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        const usize rowOffset = rowIndex * dimX;
        bool isInsideRun = false;
        usize runCount = 0;
        for(usize x = 0; x < dimX; ++x)
        {
          const bool isForeground = localPredicate(inputGroup[rowOffset + x]);
          runCount += static_cast<usize>(isForeground && !isInsideRun);
          isInsideRun = isForeground;
        }
        rowRunCounts[rowIndex] = runCount;
      }
    };
    parallelRows.execute(countRowRuns);
    if(shouldCancel)
    {
      return {};
    }

    for(usize rowIndex = 0; rowIndex < groupRows; ++rowIndex)
    {
      rowLabelOffsets[rowIndex] = provisionalLabel;
      provisionalLabel += rowRunCounts[rowIndex];
    }

    const auto mapRows = [&](const Range& range) {
      const Pred localPredicate = pred;
      for(usize rowIndex = range.min(); rowIndex < range.max(); ++rowIndex)
      {
        if(shouldCancel)
        {
          return;
        }
        bool isInsideRun = false;
        uint32 outputLabel = 0;
        usize rowLabel = rowLabelOffsets[rowIndex];
        const usize rowOffset = rowIndex * dimX;
        for(usize x = 0; x < dimX; ++x)
        {
          const bool isForeground = localPredicate(inputGroup[rowOffset + x]);
          if(isForeground && !isInsideRun)
          {
            ++rowLabel;
            outputLabel = finalLabel[rowLabel];
          }
          outputGroup[rowOffset + x] = isForeground ? outputLabel : uint32{0};
          isInsideRun = isForeground;
        }
      }
    };
    parallelRows.execute(mapRows);
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = outStore.copyFromBuffer(groupOffset, nonstd::span<const uint32>(outputGroup.data(), groupValues)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

/**
 * @brief Recreates raster-order run labels and writes the final two-dimensional label image.
 * @tparam T Input value type.
 * @tparam Pred Foreground predicate type.
 * @param inStore Input image store.
 * @param outStore Output label store.
 * @param dims Image dimensions in X, Y, Z order.
 * @param pred Predicate that identifies foreground values.
 * @param finalLabel Final label for each raster-order provisional label.
 * @param shouldCancel Shared cancellation flag.
 * @param targetBytes Maximum requested bytes for block or tile staging.
 * @return A valid result or a planning, input, or output transfer error.
 */
template <class T, class Pred>
Result<> ReplayTwoDimensionalLabels(const AbstractDataStore<T>& inStore, AbstractDataStore<uint32>& outStore, const SizeVec3& dims, const Pred& pred, const std::vector<uint32>& finalLabel,
                                    const std::atomic_bool& shouldCancel, usize targetBytes)
{
  auto planResult = CreateConnectedComponent2DPlan<T>(dims[0], dims[1], /*writesProvisional=*/true, targetBytes);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const ConnectedComponent2DPlan plan = planResult.value();
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  uint32 provisionalLabel = 0;

  if(!plan.useTiles)
  {
    const usize blockRows = AlignConnectedComponent2DRows(plan.coreRows, dimX, inStore.getChunkShape(), outStore.getChunkShape());
    const usize maximumBlockValues = blockRows * dimX;
    std::vector<T> inputBlock(maximumBlockValues);
    std::vector<uint32> outputBlock(maximumBlockValues);
    for(usize rowBegin = 0; rowBegin < dimY; rowBegin += blockRows)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(blockRows, dimY - rowBegin);
      const usize blockValues = rowCount * dimX;
      const usize blockOffset = rowBegin * dimX;
      if(Result<> result = inStore.copyIntoBuffer(blockOffset, nonstd::span<T>(inputBlock.data(), blockValues)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize localRow = 0; localRow < rowCount; ++localRow)
      {
        bool isInsideRun = false;
        uint32 outputLabel = 0;
        const usize rowOffset = localRow * dimX;
        for(usize x = 0; x < dimX; ++x)
        {
          const bool isForeground = pred(inputBlock[rowOffset + x]);
          if(isForeground && !isInsideRun)
          {
            ++provisionalLabel;
            outputLabel = finalLabel[provisionalLabel];
          }
          outputBlock[rowOffset + x] = isForeground ? outputLabel : uint32{0};
          isInsideRun = isForeground;
        }
      }
      if(Result<> result = outStore.copyFromBuffer(blockOffset, nonstd::span<const uint32>(outputBlock.data(), blockValues)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  std::vector<T> inputTile(plan.coreColumns);
  std::vector<uint32> outputTile(plan.coreColumns);
  for(usize y = 0; y < dimY; ++y)
  {
    bool isInsideRun = false;
    uint32 outputLabel = 0;
    for(usize xBegin = 0; xBegin < dimX; xBegin += plan.coreColumns)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize columnCount = std::min(plan.coreColumns, dimX - xBegin);
      const usize tileOffset = y * dimX + xBegin;
      if(Result<> result = inStore.copyIntoBuffer(tileOffset, nonstd::span<T>(inputTile.data(), columnCount)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize localX = 0; localX < columnCount; ++localX)
      {
        const bool isForeground = pred(inputTile[localX]);
        if(isForeground && !isInsideRun)
        {
          ++provisionalLabel;
          outputLabel = finalLabel[provisionalLabel];
        }
        outputTile[localX] = isForeground ? outputLabel : uint32{0};
        isInsideRun = isForeground;
      }
      if(Result<> result = outStore.copyFromBuffer(tileOffset, nonstd::span<const uint32>(outputTile.data(), columnCount)); result.invalid())
      {
        return result;
      }
    }
  }
  return {};
}

template <class T, class Pred>
Result<> Pass1TwoDimensionalBlocks(const AbstractDataStore<T>& inStore, ConnectedComponentProvisionalStore* provisional, const SizeVec3& dims, const Pred& pred, bool fullyConnected,
                                   const std::atomic_bool& shouldCancel, CcUnionFind& uf, std::vector<uint64>* sizeByLabel, const ConnectedComponent2DPlan& plan)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const int64 signedDimX = static_cast<int64>(dimX);
  const usize blockRows = provisional == nullptr ? plan.coreRows : AlignConnectedComponent2DRows(plan.coreRows, dimX, provisional->getChunkShape(), std::nullopt);
  const usize maximumBlockValues = blockRows * dimX;
  std::vector<T> inputBlock(maximumBlockValues);
  std::vector<uint32> provisionalBlock;
  if(provisional != nullptr)
  {
    provisionalBlock.resize(maximumBlockValues);
  }
  CcLine previousLine;
  CcLine currentLine;
  previousLine.reserve(plan.maximumRunsPerLine);
  currentLine.reserve(plan.maximumRunsPerLine);
  const int64 lineOffset = fullyConnected ? 1 : 0;

  for(usize rowBegin = 0; rowBegin < dimY;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize rowCount = std::min(blockRows, dimY - rowBegin);
    const usize blockValues = rowCount * dimX;
    const usize blockOffset = rowBegin * dimX;
    if(Result<> result = inStore.copyIntoBuffer(blockOffset, nonstd::span<T>(inputBlock.data(), blockValues)); result.invalid())
    {
      return result;
    }
    if(provisional != nullptr)
    {
      std::fill_n(provisionalBlock.begin(), blockValues, uint32{0});
    }

    for(usize localRow = 0; localRow < rowCount; ++localRow)
    {
      const usize rowOffset = localRow * dimX;
      EncodeLineInto<T, Pred>(nonstd::span<const T>(inputBlock.data() + rowOffset, dimX), signedDimX, pred, uf, currentLine);
      if(rowBegin + localRow > 0)
      {
        CompareLines(currentLine, previousLine, lineOffset, uf);
      }

      for(const CcRun& run : currentLine)
      {
        if(sizeByLabel != nullptr)
        {
          if(sizeByLabel->size() <= run.label)
          {
            sizeByLabel->resize(static_cast<usize>(run.label) + 1, 0u);
          }
          (*sizeByLabel)[run.label] += static_cast<uint64>(run.length);
        }
        if(provisional != nullptr)
        {
          std::fill_n(provisionalBlock.begin() + rowOffset + static_cast<usize>(run.start), static_cast<usize>(run.length), run.label);
        }
      }
      previousLine.swap(currentLine);
      currentLine.clear();
    }

    if(provisional != nullptr)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = provisional->copyFromBuffer(blockOffset, nonstd::span<const uint32>(provisionalBlock.data(), blockValues)); result.invalid())
      {
        return result;
      }
    }
    rowBegin += rowCount;
  }
  return {};
}

template <class T, class Pred>
Result<> Pass1TwoDimensionalTiles(const AbstractDataStore<T>& inStore, ConnectedComponentProvisionalStore& provisional, const SizeVec3& dims, const Pred& pred, bool fullyConnected,
                                  const std::atomic_bool& shouldCancel, CcUnionFind& uf, std::vector<uint64>* sizeByLabel, const ConnectedComponent2DPlan& plan)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  std::vector<T> inputTile(plan.coreColumns);
  std::vector<uint32> currentLabels(plan.coreColumns);
  std::vector<uint32> previousLabels(plan.coreColumns + 2);

  for(usize y = 0; y < dimY; ++y)
  {
    bool runContinues = false;
    uint32 currentRunLabel = 0;
    for(usize xBegin = 0; xBegin < dimX;)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize columnCount = std::min(plan.coreColumns, dimX - xBegin);
      if(Result<> result = inStore.copyIntoBuffer(y * dimX + xBegin, nonstd::span<T>(inputTile.data(), columnCount)); result.invalid())
      {
        return result;
      }

      usize previousBegin = 0;
      usize previousCount = 0;
      if(y > 0)
      {
        previousBegin = xBegin > 0 ? xBegin - 1 : xBegin;
        const usize coreEnd = xBegin + columnCount;
        const usize previousEnd = coreEnd < dimX ? coreEnd + 1 : dimX;
        previousCount = previousEnd - previousBegin;
        if(Result<> result = provisional.copyIntoBuffer((y - 1) * dimX + previousBegin, nonstd::span<uint32>(previousLabels.data(), previousCount)); result.invalid())
        {
          return result;
        }
      }
      std::fill_n(currentLabels.begin(), columnCount, uint32{0});

      for(usize localX = 0; localX < columnCount; ++localX)
      {
        const usize x = xBegin + localX;
        if(!pred(inputTile[localX]))
        {
          runContinues = false;
          currentRunLabel = 0;
          continue;
        }
        if(!runContinues)
        {
          currentRunLabel = uf.makeLabel();
          runContinues = true;
          if(sizeByLabel != nullptr && sizeByLabel->size() <= currentRunLabel)
          {
            sizeByLabel->resize(static_cast<usize>(currentRunLabel) + 1, 0u);
          }
        }
        currentLabels[localX] = currentRunLabel;
        if(sizeByLabel != nullptr)
        {
          ++(*sizeByLabel)[currentRunLabel];
        }

        if(y > 0)
        {
          const usize firstNeighborX = fullyConnected && x > 0 ? x - 1 : x;
          const usize lastNeighborX = fullyConnected ? std::min(dimX - 1, x + 1) : x;
          for(usize neighborX = firstNeighborX; neighborX <= lastNeighborX; ++neighborX)
          {
            const uint32 neighborLabel = previousLabels[neighborX - previousBegin];
            if(neighborLabel != 0)
            {
              uf.link(currentRunLabel, neighborLabel);
            }
          }
        }
      }

      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = provisional.copyFromBuffer(y * dimX + xBegin, nonstd::span<const uint32>(currentLabels.data(), columnCount)); result.invalid())
      {
        return result;
      }
      xBegin += columnCount;
    }
  }
  return {};
}

template <class T, class Pred>
Result<> Pass1TwoDimensional(const AbstractDataStore<T>& inStore, ConnectedComponentProvisionalStore* provisional, const SizeVec3& dims, const Pred& pred, bool fullyConnected,
                             const std::atomic_bool& shouldCancel, CcUnionFind& uf, std::vector<uint64>* sizeByLabel, usize target2DBytes)
{
  auto planResult = CreateConnectedComponent2DPlan<T>(dims[0], dims[1], provisional != nullptr, target2DBytes);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const ConnectedComponent2DPlan plan = planResult.value();
  if(plan.useTiles)
  {
    std::shared_ptr<AbstractDataStore<uint32>> tileProvisionalOwner;
    std::unique_ptr<SweepTemporaryStore<uint32>> tileFixedRecords;
    std::unique_ptr<ConnectedComponentProvisionalStore> tileAdapter;
    ConnectedComponentProvisionalStore* tileProvisional = provisional;
    if(tileProvisional == nullptr)
    {
      if(inStore.getStoreType() == IDataStore::StoreType::OutOfCore && DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
      {
        auto storeResult = CreateSweepTemporaryStore<uint32>(dims[0] * dims[1], std::max<usize>(1, target2DBytes / sizeof(uint32)), shouldCancel, "Connected-component tiled count");
        if(storeResult.invalid())
        {
          return ConvertResult(std::move(storeResult));
        }
        tileFixedRecords = std::move(storeResult.value());
        tileAdapter = std::make_unique<ConnectedComponentProvisionalStore>(*tileFixedRecords);
      }
      else
      {
        tileProvisionalOwner = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(inStore.getDataFormat(), std::vector<usize>{1, dims[1], dims[0]}, std::vector<usize>{1});
        if(tileProvisionalOwner != nullptr)
        {
          tileAdapter = std::make_unique<ConnectedComponentProvisionalStore>(*tileProvisionalOwner);
        }
      }
      tileProvisional = tileAdapter.get();
    }
    if(tileProvisional == nullptr)
    {
      return MakeErrorResult(-8375, fmt::format("Connected-component true-2-D tiled count failed to create its {}-value provisional-label scratch store.", dims[0] * dims[1]));
    }
    return Pass1TwoDimensionalTiles(inStore, *tileProvisional, dims, pred, fullyConnected, shouldCancel, uf, sizeByLabel, plan);
  }
  return Pass1TwoDimensionalBlocks(inStore, provisional, dims, pred, fullyConnected, shouldCancel, uf, sizeByLabel, plan);
}
} // namespace detail

// Full labeling writes consecutive uint32 labels and reserves zero for the background. Bounded OOC routes replay the
// input after union resolution, so they do not store a complete provisional-label volume. An extreme 2D tile route can
// use temporary records for previous-row labels. Raster-order label creation and union operations remain byte-exact.
template <class T, class Pred>
Result<> LabelConnectedComponents(const AbstractDataStore<T>& inStore, AbstractDataStore<uint32>& outStore, const SizeVec3& dims, const Pred& pred, bool fullyConnected,
                                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_ConnectedComponent2DTargetBytes)
{
  (void)messageHandler;
  usize sliceValues = 0;
  usize vol = 0;
  if(Result<> validation = detail::ValidateConnectedComponentVolume(inStore, dims, sliceValues, vol); validation.invalid())
  {
    return validation;
  }
  if(vol == 0)
  {
    return {};
  }
  if(outStore.getSize() != vol)
  {
    return MakeErrorResult(-8373, fmt::format("Connected-component output store size ({}) does not match the expected image volume ({}) for dimensions {} x {} x {}.", outStore.getSize(), vol, dims[0],
                                              dims[1], dims[2]));
  }

  const bool usesOutOfCoreEndpoint = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(usesOutOfCoreEndpoint && detail::ShouldUseConnectedComponentResidentProvisional(dims))
  {
    auto allocationResult = detail::ReserveConnectedComponentResidentWorkingMemory<T>(dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    if(allocation.holdsCompleteState())
    {
      try
      {
        DataStore<T> residentInput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
        DataStore<uint32> residentOutput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
        if(Result<> result = inStore.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
        {
          return result;
        }
        if(Result<> result = LabelConnectedComponents(residentInput, residentOutput, dims, pred, fullyConnected, shouldCancel, messageHandler, target2DBytes); result.invalid())
        {
          return result;
        }
        if(shouldCancel)
        {
          return {};
        }
        const auto outputSpan = residentOutput.createSpan();
        return outStore.copyFromBuffer(0, nonstd::span<const uint32>(outputSpan.data(), outputSpan.size()));
      } catch(const std::bad_alloc&)
      {
        // Release the full-state reservation before trying the resident-provisional and bounded tiers.
      }
    }
  }
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);

  std::optional<detail::ConnectedComponentProvisionalSelection> provisionalSelection;
  std::shared_ptr<AbstractDataStore<uint32>> provisionalDataStore;
  std::unique_ptr<detail::ConnectedComponentProvisionalStore> provisional;
  auto residentSelectionResult = detail::TryCreateConnectedComponentResidentProvisionalSelection(dims, usesOutOfCoreEndpoint);
  if(residentSelectionResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(residentSelectionResult));
  }
  if(residentSelectionResult.value().holdsResidentState())
  {
    provisionalSelection.emplace(std::move(residentSelectionResult.value()));
    provisionalDataStore = provisionalSelection->dataStore;
    provisional = std::make_unique<detail::ConnectedComponentProvisionalStore>(*provisionalDataStore);
  }
  if(provisional == nullptr && !usesOutOfCoreEndpoint)
  {
    const std::string provisionalDataFormat = detail::SelectConnectedComponentProvisionalDataFormat(inStore, outStore);
    provisionalDataStore = DataStoreUtilities::CreateDataStoreWithFormat<uint32>(provisionalDataFormat, ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1});
    if(provisionalDataStore == nullptr)
    {
      return MakeErrorResult(-8375, fmt::format("Connected-component labeling failed to create its {}-value provisional-label scratch store.", vol));
    }
    provisional = std::make_unique<detail::ConnectedComponentProvisionalStore>(*provisionalDataStore);
  }

  detail::CcUnionFind uf;
  Result<> pass1Result = nZ == 1 ? detail::Pass1TwoDimensional<T, Pred>(inStore, provisional.get(), dims, pred, fullyConnected, shouldCancel, uf, nullptr, target2DBytes) :
                                   detail::Pass1ThreeDimensional<T, Pred>(inStore, provisional.get(), dims, pred, fullyConnected, shouldCancel, uf, nullptr);
  if(pass1Result.invalid())
  {
    return pass1Result;
  }
  if(shouldCancel)
  {
    return {};
  }

  // ---- Resolve: CreateConsecutive, then a direct per-provisional-label lookup so pass 2 is O(1) per voxel. ----
  const std::pair<std::vector<uint32>, uint32> resolved = uf.createConsecutive();
  const std::vector<uint32>& consecutive = resolved.first;
  std::vector<uint32> finalLabel(uf.parent.size(), 0u);
  for(usize i = 1; i < uf.parent.size(); ++i)
  {
    finalLabel[i] = consecutive[uf.find(static_cast<uint32>(i))];
  }

  if(provisional == nullptr)
  {
    return nZ == 1 ? detail::ReplayTwoDimensionalLabels(inStore, outStore, dims, pred, finalLabel, shouldCancel, target2DBytes) :
                     detail::ReplayThreeDimensionalLabels(inStore, outStore, dims, pred, finalLabel, shouldCancel);
  }

  // ---- Pass 2: relabel the provisional scratch into consecutive output labels. ----
  if(nZ == 1)
  {
    auto chunkResult = detail::CreateConnectedComponent2DFinalChunkValues(vol, dims[0], target2DBytes, provisional->getChunkShape(), outStore.getChunkShape());
    if(chunkResult.invalid())
    {
      return ConvertResult(std::move(chunkResult));
    }
    const usize chunkValues = chunkResult.value();
    std::vector<uint32> labels(chunkValues);
    for(usize start = 0; start < vol;)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(chunkValues, vol - start);
      if(Result<> result = provisional->copyIntoBuffer(start, nonstd::span<uint32>(labels.data(), count)); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < count; ++index)
      {
        labels[index] = finalLabel[labels[index]];
      }
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> result = outStore.copyFromBuffer(start, nonstd::span<const uint32>(labels.data(), count)); result.invalid())
      {
        return result;
      }
      start += count;
    }
    return {};
  }

  std::vector<uint32> provPlane(sliceValues);
  std::vector<uint32> outPlane(sliceValues);
  detail::ConnectedComponentProvisionalStore& provisionalPlaneStore = *provisional;
  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize planeOffset = static_cast<usize>(z) * sliceValues;
    if(Result<> r = provisionalPlaneStore.copyIntoBuffer(planeOffset, nonstd::span<uint32>(provPlane.data(), sliceValues)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < sliceValues; ++i)
    {
      outPlane[i] = finalLabel[provPlane[i]]; // finalLabel[0] is always 0 (background), so no branch is needed here
    }
    if(Result<> r = outStore.copyFromBuffer(planeOffset, nonstd::span<const uint32>(outPlane.data(), sliceValues)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

// Count-only: number of components with size >= minSize. Pass 1 + resolve + per-root size tally, no output written.
// Returns Result<uint32> (rather than a bare uint32) so a Pass1 I/O failure is distinguishable from a legitimate
// "0 components" count.
template <class T, class Pred>
Result<uint32> CountConnectedComponents(const AbstractDataStore<T>& inStore, const SizeVec3& dims, const Pred& pred, bool fullyConnected, uint64 minSize, const std::atomic_bool& shouldCancel,
                                        usize target2DBytes = detail::k_ConnectedComponent2DTargetBytes)
{
  usize sliceValues = 0;
  usize volumeValues = 0;
  if(Result<> validation = detail::ValidateConnectedComponentVolume(inStore, dims, sliceValues, volumeValues); validation.invalid())
  {
    return ConvertResultTo<uint32>(std::move(validation), uint32{0});
  }
  if(volumeValues == 0)
  {
    return Result<uint32>{0u};
  }
  detail::CcUnionFind uf;
  std::vector<uint64> sizeByLabel; // sizeByLabel[provisional label] = total run length for that (pre-union) label
  Result<> pass1Result = dims[2] == 1 ? detail::Pass1TwoDimensional<T, Pred>(inStore, nullptr, dims, pred, fullyConnected, shouldCancel, uf, &sizeByLabel, target2DBytes) :
                                        detail::Pass1ThreeDimensional<T, Pred>(inStore, nullptr, dims, pred, fullyConnected, shouldCancel, uf, &sizeByLabel);
  if(pass1Result.invalid())
  {
    return ConvertResultTo<uint32>(std::move(pass1Result), uint32{0});
  }
  if(shouldCancel)
  {
    return Result<uint32>{0u};
  }

  // Resolve per-root sizes now that the union-find is final (roots can still change during Pass1, so this can only be
  // done AFTER Pass1 completes -- summing by raw provisional label first, as Pass1 does, is what makes that safe).
  std::vector<uint64> rootSize(uf.parent.size(), 0u);
  for(usize label = 1; label < sizeByLabel.size(); ++label)
  {
    rootSize[uf.find(static_cast<uint32>(label))] += sizeByLabel[label];
  }

  uint32 numComponents = 0;
  for(usize i = 1; i < uf.parent.size(); ++i)
  {
    if(uf.parent[i] == i && rootSize[i] >= minSize) // i is a root (one entry per component)
    {
      ++numComponents;
    }
  }
  return Result<uint32>{numComponents};
}
} // namespace nx::core::ImageProcessing
