#include "TriangleUtilities.hpp"

#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <nonstd/span.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <list>
#include <memory>
#include <new>
#include <queue>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
using EdgeListT = std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>>;

inline constexpr uint64 k_WindingBatchRecords = 4096;
inline constexpr usize k_WindingCachePages = 8;

/**
 * @struct VertexOccurrence
 * @brief Stores one vertex use by a triangle corner.
 */
struct VertexOccurrence
{
  IGeometry::MeshIndexType Vertex = 0;
  uint64 Triangle = 0;
  uint8 Ordinal = 0;
};

/**
 * @struct NeighborContribution
 * @brief Stores one candidate-neighbor contribution to a source triangle.
 */
struct NeighborContribution
{
  uint64 Source = 0;
  uint64 Candidate = 0;
  uint8 SourceOrdinal = 0;
};

/**
 * @struct WindingNeighbor
 * @brief Stores one ordered triangle-neighbor relation.
 */
struct WindingNeighbor
{
  uint64 Source = 0;
  uint64 Candidate = 0;
  uint8 SourceOrdinal = 0;
};

/**
 * @struct FeatureSeed
 * @brief Stores one candidate traversal seed for a feature label.
 */
struct FeatureSeed
{
  int32 Feature = 0;
  uint64 Triangle = 0;
  uint8 Component = 0;
};

/**
 * @struct NeighborIndex
 * @brief Stores one source triangle's range in sorted neighbor records.
 */
struct NeighborIndex
{
  uint64 First = 0;
  uint64 Count = 0;
};

/**
 * @struct TriangleWindingState
 * @brief Stores traversal and flip state for one triangle.
 */
struct TriangleWindingState
{
  uint8 Visited = 0;
  uint8 Unmodified = 0;
};

static_assert(std::is_trivially_copyable_v<VertexOccurrence>);
static_assert(std::is_trivially_copyable_v<NeighborContribution>);
static_assert(std::is_trivially_copyable_v<WindingNeighbor>);
static_assert(std::is_trivially_copyable_v<FeatureSeed>);
static_assert(std::is_trivially_copyable_v<NeighborIndex>);
static_assert(std::is_trivially_copyable_v<TriangleWindingState>);

/**
 * @brief Decodes one trivially copyable record from bytes.
 * @tparam T Specifies the record type.
 * @param bytes Provides at least sizeof(T) bytes.
 * @return Decoded record.
 */
template <typename T>
T DecodeWindingRecord(nonstd::span<const std::byte> bytes)
{
  T record{};
  std::memcpy(&record, bytes.data(), sizeof(T));
  return record;
}

/**
 * @brief Compares two sortable record values.
 * @tparam T Specifies an ordered value type.
 * @param left Provides the first value.
 * @param right Provides the second value.
 * @return -1, 0, or 1 for ascending order.
 */
template <typename T>
int32 CompareWindingValue(const T& left, const T& right)
{
  if(left < right)
  {
    return -1;
  }
  if(right < left)
  {
    return 1;
  }
  return 0;
}

/**
 * @brief Compares vertex-occurrence records for external sorting.
 * @param leftBytes Provides the first record.
 * @param rightBytes Provides the second record.
 * @return Ascending comparison by vertex, triangle, and ordinal.
 */
int32 CompareVertexOccurrences(nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes)
{
  const VertexOccurrence left = DecodeWindingRecord<VertexOccurrence>(leftBytes);
  const VertexOccurrence right = DecodeWindingRecord<VertexOccurrence>(rightBytes);
  if(const int32 vertexComparison = CompareWindingValue(left.Vertex, right.Vertex); vertexComparison != 0)
  {
    return vertexComparison;
  }
  if(const int32 triangleComparison = CompareWindingValue(left.Triangle, right.Triangle); triangleComparison != 0)
  {
    return triangleComparison;
  }
  return CompareWindingValue(left.Ordinal, right.Ordinal);
}

/**
 * @brief Compares neighbor-contribution records for external sorting.
 * @param leftBytes Provides the first record.
 * @param rightBytes Provides the second record.
 * @return Ascending comparison by source, candidate, and ordinal.
 */
int32 CompareNeighborContributions(nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes)
{
  const NeighborContribution left = DecodeWindingRecord<NeighborContribution>(leftBytes);
  const NeighborContribution right = DecodeWindingRecord<NeighborContribution>(rightBytes);
  if(const int32 sourceComparison = CompareWindingValue(left.Source, right.Source); sourceComparison != 0)
  {
    return sourceComparison;
  }
  if(const int32 candidateComparison = CompareWindingValue(left.Candidate, right.Candidate); candidateComparison != 0)
  {
    return candidateComparison;
  }
  return CompareWindingValue(left.SourceOrdinal, right.SourceOrdinal);
}

/**
 * @brief Compares ordered winding-neighbor records for external sorting.
 * @param leftBytes Provides the first record.
 * @param rightBytes Provides the second record.
 * @return Ascending comparison by source, ordinal, and candidate.
 */
int32 CompareWindingNeighbors(nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes)
{
  const WindingNeighbor left = DecodeWindingRecord<WindingNeighbor>(leftBytes);
  const WindingNeighbor right = DecodeWindingRecord<WindingNeighbor>(rightBytes);
  if(const int32 sourceComparison = CompareWindingValue(left.Source, right.Source); sourceComparison != 0)
  {
    return sourceComparison;
  }
  if(const int32 ordinalComparison = CompareWindingValue(left.SourceOrdinal, right.SourceOrdinal); ordinalComparison != 0)
  {
    return ordinalComparison;
  }
  return CompareWindingValue(left.Candidate, right.Candidate);
}

/**
 * @brief Compares feature-seed records for external sorting.
 * @param leftBytes Provides the first record.
 * @param rightBytes Provides the second record.
 * @return Ascending comparison by feature, triangle, and component.
 */
int32 CompareFeatureSeeds(nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes)
{
  const FeatureSeed left = DecodeWindingRecord<FeatureSeed>(leftBytes);
  const FeatureSeed right = DecodeWindingRecord<FeatureSeed>(rightBytes);
  if(const int32 featureComparison = CompareWindingValue(left.Feature, right.Feature); featureComparison != 0)
  {
    return featureComparison;
  }
  if(const int32 triangleComparison = CompareWindingValue(left.Triangle, right.Triangle); triangleComparison != 0)
  {
    return triangleComparison;
  }
  return CompareWindingValue(left.Component, right.Component);
}

/**
 * @brief Creates an external sorter for one winding record type.
 * @tparam T Specifies the record type.
 * @param compare Specifies record ordering.
 * @return External sorter or provider error.
 */
template <typename T>
Result<std::unique_ptr<IExternalSort>> CreateWindingSort(ExternalSortCompare compare)
{
  ExternalSortConfig config;
  config.recordSize = sizeof(T);
  config.maxRecordsPerBatch = k_WindingBatchRecords;
  config.compare = std::move(compare);
  auto result = DataStoreUtilities::GetIOCollection().createExternalSort(config);
  if(result.valid() && result.value() == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<IExternalSort>>(-65780, "Triangle winding external-sort provider returned a null sorter.");
  }
  return result;
}

/**
 * @brief Creates a temporary store for one winding record type.
 * @tparam T Specifies the record type.
 * @param recordCount Specifies initial records.
 * @return Temporary store or provider error.
 */
template <typename T>
Result<std::unique_ptr<ITemporaryRecordStore>> CreateWindingRecordStore(uint64 recordCount)
{
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(T);
  config.maxRecordsPerBatch = k_WindingBatchRecords;
  config.initialRecordCount = recordCount;
  auto result = DataStoreUtilities::GetIOCollection().createTemporaryRecordStore(config);
  if(result.valid() && result.value() == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-65781, "Triangle winding temporary-record provider returned a null store.");
  }
  return result;
}

/**
 * @class WindingSortAppender
 * @brief Appends typed winding records through a bounded batch buffer.
 * @tparam T Specifies the record type.
 */
template <typename T>
class WindingSortAppender
{
public:
  /**
   * @brief Creates a buffered appender for one external sorter.
   * @param sorter Receives record batches.
   * @param shouldCancel Stops sorter operations when true.
   */
  WindingSortAppender(IExternalSort& sorter, const std::atomic_bool& shouldCancel)
  : m_Sorter(sorter)
  , m_ShouldCancel(shouldCancel)
  {
    m_Buffer.reserve(k_WindingBatchRecords);
  }

  /**
   * @brief Adds one record and flushes a full batch.
   * @param record Specifies the record.
   * @return Allocation or sorter-append error, or success.
   */
  Result<> append(const T& record)
  {
    try
    {
      m_Buffer.push_back(record);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-65782, "Triangle winding failed to allocate a bounded external-sort append buffer.");
    }
    if(m_Buffer.size() == k_WindingBatchRecords)
    {
      return flush();
    }
    return {};
  }

  /**
   * @brief Flushes the current record batch.
   * @return Sorter-append error or success.
   */
  Result<> flush()
  {
    if(m_Buffer.empty())
    {
      return {};
    }
    const auto bytes = nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(m_Buffer.data()), m_Buffer.size() * sizeof(T));
    auto result = m_Sorter.append(static_cast<uint64>(m_Buffer.size()), bytes, m_ShouldCancel, {});
    if(result.valid())
    {
      m_Buffer.clear();
    }
    return result;
  }

private:
  IExternalSort& m_Sorter;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<T> m_Buffer;
};

/**
 * @class WindingSortReader
 * @brief Reads sorted winding records through one bounded page.
 * @tparam T Specifies the record type.
 */
template <typename T>
class WindingSortReader
{
public:
  /**
   * @brief Creates a paged reader for one completed sorter.
   * @param sorter Provides sorted records.
   * @param shouldCancel Stops sorter reads when true.
   */
  WindingSortReader(const IExternalSort& sorter, const std::atomic_bool& shouldCancel)
  : m_Sorter(sorter)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Reads one sorted record.
   * @param index Specifies the record index.
   * @return Record or range, allocation, or provider error.
   */
  Result<T> read(uint64 index)
  {
    if(index >= m_Sorter.recordCount())
    {
      return MakeErrorResult<T>(-65783, "Triangle winding external-sort read exceeded the record count.");
    }
    if(index < m_PageFirst || index >= m_PageFirst + m_PageCount)
    {
      m_PageFirst = (index / k_WindingBatchRecords) * k_WindingBatchRecords;
      m_PageCount = std::min<uint64>(k_WindingBatchRecords, m_Sorter.recordCount() - m_PageFirst);
      try
      {
        m_Page.resize(static_cast<usize>(m_PageCount));
      } catch(const std::bad_alloc&)
      {
        return MakeErrorResult<T>(-65784, "Triangle winding failed to allocate a bounded external-sort read buffer.");
      } catch(const std::length_error&)
      {
        return MakeErrorResult<T>(-65784, "Triangle winding failed to allocate a bounded external-sort read buffer.");
      }
      auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(m_Page.data()), m_Page.size() * sizeof(T));
      auto result = m_Sorter.read(m_PageFirst, m_PageCount, bytes, m_ShouldCancel);
      if(result.invalid())
      {
        return ConvertInvalidResult<T>(std::move(result));
      }
      if(result.value() != m_PageCount)
      {
        return MakeErrorResult<T>(-65785, "Triangle winding external-sort provider returned a short read.");
      }
    }
    return {m_Page[static_cast<usize>(index - m_PageFirst)]};
  }

private:
  const IExternalSort& m_Sorter;
  const std::atomic_bool& m_ShouldCancel;
  uint64 m_PageFirst = std::numeric_limits<uint64>::max();
  uint64 m_PageCount = 0;
  std::vector<T> m_Page;
};

/**
 * @class WindingDataStoreCache
 * @brief Provides bounded LRU tuple access to one DataStore.
 * @tparam T Specifies the scalar type.
 */
template <typename T>
class WindingDataStoreCache
{
public:
  /**
   * @brief Creates a read-only tuple cache.
   * @param store Provides source tuples.
   * @param tuplesPerPage Specifies page tuple count.
   * @param maximumPages Limits resident pages.
   */
  WindingDataStoreCache(const AbstractDataStore<T>& store, uint64 tuplesPerPage, usize maximumPages)
  : m_Store(store)
  , m_TuplesPerPage(tuplesPerPage)
  , m_MaximumPages(maximumPages)
  {
  }

  /**
   * @brief Creates a mutable tuple cache.
   * @param store Provides and receives tuples.
   * @param tuplesPerPage Specifies page tuple count.
   * @param maximumPages Limits resident pages.
   */
  WindingDataStoreCache(AbstractDataStore<T>& store, uint64 tuplesPerPage, usize maximumPages)
  : m_Store(store)
  , m_MutableStore(&store)
  , m_TuplesPerPage(tuplesPerPage)
  , m_MaximumPages(maximumPages)
  {
  }

  /**
   * @brief Reads one tuple through the cache.
   * @param tupleIndex Specifies the tuple.
   * @param tuple Receives all tuple components.
   * @return Shape, range, allocation, or source-read error, or success.
   */
  Result<> readTuple(uint64 tupleIndex, nonstd::span<T> tuple)
  {
    if(tuple.size() != m_Store.getNumberOfComponents())
    {
      return MakeErrorResult(-65786, "Triangle winding DataStore cache received a tuple with the wrong component count.");
    }
    auto pageResult = loadPage(tupleIndex);
    if(pageResult.invalid())
    {
      return ConvertResult(std::move(pageResult));
    }
    const Page& page = pageResult.value().get();
    const usize localTuple = static_cast<usize>(tupleIndex - page.FirstTuple);
    std::copy_n(page.Values.data() + localTuple * tuple.size(), tuple.size(), tuple.data());
    return {};
  }

  /**
   * @brief Updates one cached tuple and marks its page dirty.
   * @param tupleIndex Specifies the tuple.
   * @param tuple Provides all tuple components.
   * @return Mutability, shape, range, allocation, or source-read error, or success.
   */
  Result<> writeTuple(uint64 tupleIndex, nonstd::span<const T> tuple)
  {
    if(m_MutableStore == nullptr)
    {
      return MakeErrorResult(-65787, "Triangle winding attempted to modify a read-only DataStore cache.");
    }
    if(tuple.size() != m_Store.getNumberOfComponents())
    {
      return MakeErrorResult(-65786, "Triangle winding DataStore cache received a tuple with the wrong component count.");
    }
    auto pageResult = loadPage(tupleIndex);
    if(pageResult.invalid())
    {
      return ConvertResult(std::move(pageResult));
    }
    Page& page = pageResult.value().get();
    const usize localTuple = static_cast<usize>(tupleIndex - page.FirstTuple);
    std::copy(tuple.begin(), tuple.end(), page.Values.data() + localTuple * tuple.size());
    page.Dirty = true;
    return {};
  }

  /**
   * @brief Flushes all dirty pages.
   * @return First destination-write error, or success.
   */
  Result<> flush()
  {
    for(Page& page : m_Pages)
    {
      auto result = flushPage(page);
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }

private:
  /**
   * @struct Page
   * @brief Stores one tuple page and its dirty state.
   */
  struct Page
  {
    uint64 FirstTuple = 0;
    uint64 TupleCount = 0;
    bool Dirty = false;
    std::vector<T> Values;
  };

  /**
   * @brief Returns or loads the page for one tuple.
   * @param tupleIndex Specifies the tuple.
   * @return Page reference or cache error.
   */
  Result<std::reference_wrapper<Page>> loadPage(uint64 tupleIndex)
  {
    const uint64 tupleCount = static_cast<uint64>(m_Store.getNumberOfTuples());
    const uint64 componentCount = static_cast<uint64>(m_Store.getNumberOfComponents());
    if(m_TuplesPerPage == 0 || m_MaximumPages == 0 || componentCount == 0 || tupleIndex >= tupleCount)
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-65788, "Triangle winding DataStore page-cache request is invalid.");
    }
    const uint64 firstTuple = (tupleIndex / m_TuplesPerPage) * m_TuplesPerPage;
    auto found = std::find_if(m_Pages.begin(), m_Pages.end(), [firstTuple](const Page& page) { return page.FirstTuple == firstTuple; });
    if(found != m_Pages.end())
    {
      m_Pages.splice(m_Pages.begin(), m_Pages, found);
      return {std::ref(m_Pages.front())};
    }
    if(m_Pages.size() == m_MaximumPages)
    {
      auto flushResult = flushPage(m_Pages.back());
      if(flushResult.invalid())
      {
        return ConvertInvalidResult<std::reference_wrapper<Page>>(std::move(flushResult));
      }
      m_Pages.pop_back();
    }
    Page page;
    page.FirstTuple = firstTuple;
    page.TupleCount = std::min<uint64>(m_TuplesPerPage, tupleCount - firstTuple);
    if(page.TupleCount > std::numeric_limits<usize>::max() / componentCount)
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-65789, "Triangle winding DataStore page size exceeds addressable memory.");
    }
    try
    {
      page.Values.resize(static_cast<usize>(page.TupleCount * componentCount));
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-65790, "Triangle winding failed to allocate a bounded DataStore page.");
    } catch(const std::length_error&)
    {
      return MakeErrorResult<std::reference_wrapper<Page>>(-65790, "Triangle winding failed to allocate a bounded DataStore page.");
    }
    auto readResult = m_Store.copyIntoBuffer(static_cast<usize>(firstTuple * componentCount), nonstd::span<T>(page.Values.data(), page.Values.size()));
    if(readResult.invalid())
    {
      return ConvertInvalidResult<std::reference_wrapper<Page>>(std::move(readResult));
    }
    m_Pages.push_front(std::move(page));
    return {std::ref(m_Pages.front())};
  }

  /**
   * @brief Writes one dirty page to the mutable store.
   * @param page Provides cached values and destination range.
   * @return Mutability or destination-write error, or success.
   */
  Result<> flushPage(Page& page)
  {
    if(!page.Dirty)
    {
      return {};
    }
    if(m_MutableStore == nullptr)
    {
      return MakeErrorResult(-65787, "Triangle winding attempted to flush a read-only DataStore cache.");
    }
    const usize componentCount = m_Store.getNumberOfComponents();
    auto result = m_MutableStore->copyFromBuffer(static_cast<usize>(page.FirstTuple) * componentCount, nonstd::span<const T>(page.Values.data(), page.Values.size()));
    if(result.valid())
    {
      page.Dirty = false;
    }
    return result;
  }

  const AbstractDataStore<T>& m_Store;
  AbstractDataStore<T>* m_MutableStore = nullptr;
  uint64 m_TuplesPerPage = 0;
  usize m_MaximumPages = 0;
  std::list<Page> m_Pages;
};

/**
 * @brief Finalizes one external winding sort.
 * @param sorter Provides appended records.
 * @param shouldCancel Stops sort work when true.
 * @return Sorter error or success.
 */
Result<> FinishWindingSort(IExternalSort& sorter, const std::atomic_bool& shouldCancel)
{
  return sorter.finish(shouldCancel, {});
}

/**
 * @brief Tests whether two triangle windings use a shared edge in the same direction.
 * @param triangle Provides source connectivity.
 * @param neighbor Provides adjacent connectivity.
 * @param neighborUnmodified Applies synthetic reversal when true.
 * @return True when directed shared edges conflict.
 */
bool DirectedEdgesConflict(const std::array<IGeometry::MeshIndexType, 3>& triangle, const std::array<IGeometry::MeshIndexType, 3>& neighbor, bool neighborUnmodified)
{
  const std::array<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>, 3> triangleEdges = {std::make_pair(triangle[0], triangle[1]), std::make_pair(triangle[1], triangle[2]),
                                                                                                      std::make_pair(triangle[2], triangle[0])};
  const std::array<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>, 3> neighborEdges =
      neighborUnmodified ? std::array{std::make_pair(neighbor[0], neighbor[2]), std::make_pair(neighbor[2], neighbor[1]), std::make_pair(neighbor[1], neighbor[0])} :
                           std::array{std::make_pair(neighbor[0], neighbor[1]), std::make_pair(neighbor[1], neighbor[2]), std::make_pair(neighbor[2], neighbor[0])};
  for(const auto& triangleEdge : triangleEdges)
  {
    if(std::find(neighborEdges.begin(), neighborEdges.end(), triangleEdge) != neighborEdges.end())
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief Repairs resident triangle winding independently for each face label.
 * @param triangles Provides and receives flat triangle connectivity.
 * @param numTris Specifies triangle count.
 * @param neighbors Provides adjacent triangles.
 * @param faceLabels Provides two labels per triangle.
 * @param shouldCancel Stops before later traversal work when true.
 * @param mesgHandler Receives progress messages.
 * @param maxFeature Specifies the largest positive label.
 * @return Warning for unrepaired triangles, or success after cancellation.
 * @pre The mesh has no duplicate vertices.
 */
Result<> ProcessWindingsWithLabels(IGeometry::MeshIndexType* triangles, usize numTris, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors, const int32* faceLabels,
                                   const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
{
  // Process each label separately because three-feature junction edges are not manifold.
  usize count = 0;
  auto start = std::chrono::steady_clock::now();
  std::vector<bool> visited(numTris, false);
  std::vector<bool> unmodified(numTris, false);

  // Find each feature's first triangle in one ascending pass. This preserves
  // deterministic seeds without a feature-by-triangle search.
  std::vector<int64> firstSeedPerFeature(maxFeature + 1, -1);
  for(usize i = 0; i < numTris; i++)
  {
    const int32 label0 = faceLabels[i * 2];
    if(label0 >= 1 && label0 <= maxFeature && firstSeedPerFeature[label0] < 0)
    {
      firstSeedPerFeature[label0] = static_cast<int64>(i);
    }
    const int32 label1 = faceLabels[(i * 2) + 1];
    if(label1 >= 1 && label1 <= maxFeature && firstSeedPerFeature[label1] < 0)
    {
      firstSeedPerFeature[label1] = static_cast<int64>(i);
    }
  }

  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // Start traversal from the precomputed seed for this feature.
    const int64 seed = firstSeedPerFeature[feature];
    if(seed >= 0)
    {
      const usize i = static_cast<usize>(seed);
      auto numElem = neighbors.getNumberOfElements(i);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(i);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabels[neighbor * 2] != feature && faceLabels[(neighbor * 2) + 1] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[i] = true;
    }

    while(!searchTargets.empty())
    {
      if(shouldCancel)
      {
        return {};
      }

      const IGeometry::MeshIndexType triangle = searchTargets.front();
      searchTargets.pop();

      if(visited[triangle])
      {
        continue;
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > 1000)
      {
        mesgHandler(fmt::format("Current Feature: {}/{} | Progress : {:2.2f}%", feature, maxFeature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabels[neighbor * 2] != feature && faceLabels[(neighbor * 2) + 1] != feature)
        {
          continue;
        }

        searchTargets.push(neighbor);
        localNeighbors.emplace(neighbor);
      }

      visited[triangle] = true;

      // Collect directed edges from adjacent triangles already visited.
      EdgeListT edgeList = {};
      for(const usize neighbor : localNeighbors)
      {
        if(!visited[neighbor])
        {
          continue;
        }

        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1 = std::make_pair(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 1]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2 = std::make_pair(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 2]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3 = std::make_pair(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 0]);

        if(unmodified[neighbor])
        {
          // Apply the stored synthetic reversal for this feature traversal.
          edge1 = std::make_pair(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 2]);
          edge2 = std::make_pair(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 1]);
          edge3 = std::make_pair(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 0]);
        }

        edgeList.emplace(std::move(edge1));
        edgeList.emplace(std::move(edge2));
        edgeList.emplace(std::move(edge3));
      }

      if(edgeList.find(std::make_pair(triangles[(triangle * 3) + 0], triangles[(triangle * 3) + 1])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 1], triangles[(triangle * 3) + 2])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 2], triangles[(triangle * 3) + 0])) != edgeList.end()) // If true it contains a conflicting edge
      {
        const usize offset = faceLabels[triangle * 2] == feature ? 1 : 0;
        const int32 alternateLabel = faceLabels[(triangle * 2) + offset];
        if(alternateLabel != 0 && alternateLabel < feature)
        {
          unmodified[triangle] = true;
          count++;
        }
        else
        {
          // Reverse connectivity when no earlier feature owns this decision.
          const IGeometry::MeshIndexType tempValue = triangles[(triangle * 3) + 0];
          triangles[(triangle * 3) + 0] = triangles[(triangle * 3) + 2];
          triangles[(triangle * 3) + 2] = tempValue;
        }
      }
    }
  }

  if(count > 0)
  {
    return MakeWarningVoidResult(-56730, fmt::format("{} triangles cold not be made consistent, due to the nature of mesh implementation.", count));
  }

  return {};
}

/**
 * @brief Repairs resident triangle winding independently for each region ID.
 * @param triangles Provides and receives flat triangle connectivity.
 * @param numTris Specifies triangle count.
 * @param neighbors Provides adjacent triangles.
 * @param regions Provides one region ID per triangle.
 * @param shouldCancel Stops before later traversal work when true.
 * @param mesgHandler Receives progress messages.
 * @param maxFeature Specifies the largest positive region ID.
 * @return Success after completion or cancellation.
 * @pre The mesh has no duplicate vertices.
 */
Result<> ProcessWindingsWithRegions(IGeometry::MeshIndexType* triangles, usize numTris, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors, const int32* regions,
                                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
{
  // Process each region separately because multi-region junction edges are not manifold.
  auto start = std::chrono::steady_clock::now();
  std::vector<bool> visited(numTris, false);

  // Find each region's first triangle in one ascending pass. This preserves
  // deterministic seeds without a region-by-triangle search.
  std::vector<int64> firstSeedPerFeature(maxFeature + 1, -1);
  for(usize i = 0; i < numTris; i++)
  {
    const int32 region = regions[i];
    if(region >= 1 && region <= maxFeature && firstSeedPerFeature[region] < 0)
    {
      firstSeedPerFeature[region] = static_cast<int64>(i);
    }
  }

  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // Start traversal from the precomputed seed for this region.
    const int64 seed = firstSeedPerFeature[feature];
    if(seed >= 0)
    {
      const usize i = static_cast<usize>(seed);
      auto numElem = neighbors.getNumberOfElements(i);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(i);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regions[neighbor] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[i] = true;
    }

    while(!searchTargets.empty())
    {
      if(shouldCancel)
      {
        return {};
      }

      const IGeometry::MeshIndexType triangle = searchTargets.front();
      searchTargets.pop();

      if(visited[triangle])
      {
        continue;
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > 1000)
      {
        mesgHandler(fmt::format("Current Feature: {}/{} | Progress : {:2.2f}%", feature, maxFeature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regions[neighbor] != feature)
        {
          continue;
        }

        searchTargets.push(neighbor);
        localNeighbors.emplace(neighbor);
      }

      visited[triangle] = true;

      // Collect directed edges from adjacent triangles already visited.
      EdgeListT edgeList = {};
      for(const usize neighbor : localNeighbors)
      {
        if(!visited[neighbor])
        {
          continue;
        }

        edgeList.emplace(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 1]);
        edgeList.emplace(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 2]);
        edgeList.emplace(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 0]);
      }

      if(edgeList.find(std::make_pair(triangles[(triangle * 3) + 0], triangles[(triangle * 3) + 1])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 1], triangles[(triangle * 3) + 2])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 2], triangles[(triangle * 3) + 0])) != edgeList.end()) // If true it contains a conflicting edge
      {
        // Reverse connectivity when a directed shared edge conflicts.
        const IGeometry::MeshIndexType tempValue = triangles[(triangle * 3) + 0];
        triangles[(triangle * 3) + 0] = triangles[(triangle * 3) + 2];
        triangles[(triangle * 3) + 2] = tempValue;
      }
    }
  }

  return {};
}
} // namespace

INodeGeometry2D::SharedVertexList::value_type MeshingUtilities::detail::FindTriangleVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices)
{
  const usize vertAIndex = vertIndices[0] * 3;
  const usize vertBIndex = vertIndices[1] * 3;
  const usize vertCIndex = vertIndices[2] * 3;

  // The 3 by 3 matrix uses row-major C order.
  std::array<INodeGeometry2D::SharedVertexList::value_type, 9> volumeMatrix = {
      vertices[vertBIndex + 0] - vertices[vertAIndex + 0], vertices[vertCIndex + 0] - vertices[vertAIndex + 0], 0.0f - vertices[vertAIndex + 0],
      vertices[vertBIndex + 1] - vertices[vertAIndex + 1], vertices[vertCIndex + 1] - vertices[vertAIndex + 1], 0.0f - vertices[vertAIndex + 1],
      vertices[vertBIndex + 2] - vertices[vertAIndex + 2], vertices[vertCIndex + 2] - vertices[vertAIndex + 2], 0.0f - vertices[vertAIndex + 2]};

  const INodeGeometry2D::SharedVertexList::value_type determinant =
      (volumeMatrix[MeshingUtilities::detail::k_00] *
       (volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_21])) -
      (volumeMatrix[MeshingUtilities::detail::k_01] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_20])) +
      (volumeMatrix[MeshingUtilities::detail::k_02] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_21] - volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_20]));
  return determinant / 6.0f;
}

Result<> MeshingUtilities::RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                                 const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
{
  usize numComp = idsStore.getNumberOfComponents();
  if(numComp > 2 || numComp == 0)
  {
    return MakeErrorResult(-65770,
                           fmt::format("MeshingUtilities::RepairTriangleWinding: invalid ID array supplied. The ID array must have 1 or 2 components, supplied array components: {}.", numComp));
  }

  const usize numTris = triangles.getNumberOfTuples();
  const usize idsSize = idsStore.getSize(); // numTris * numComp

  // Read all triangles before direct traversal.
  auto triBuf = std::make_unique<IGeometry::MeshIndexType[]>(numTris * 3);
  auto triangleReadResult = triangles.copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(triBuf.get(), numTris * 3));
  if(triangleReadResult.invalid())
  {
    return triangleReadResult;
  }

  // Read all IDs before direct traversal.
  auto idsBuf = std::make_unique<int32[]>(idsSize);
  auto idReadResult = idsStore.copyIntoBuffer(0, nonstd::span<int32>(idsBuf.get(), idsSize));
  if(idReadResult.invalid())
  {
    return idReadResult;
  }

  // Determine the traversal feature range from local IDs.
  int32 maxFeature = 0;
  for(usize i = 0; i < idsSize; i++)
  {
    maxFeature = std::max(idsBuf[i], maxFeature);
  }

  Result<> result;
  if(numComp == 2)
  {
    result = ::ProcessWindingsWithLabels(triBuf.get(), numTris, neighbors, idsBuf.get(), shouldCancel, mesgHandler, maxFeature);
  }
  else
  {
    result = ::ProcessWindingsWithRegions(triBuf.get(), numTris, neighbors, idsBuf.get(), shouldCancel, mesgHandler, maxFeature);
  }

  // Write repaired triangles in one transfer.
  auto triangleWriteResult = triangles.copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(triBuf.get(), numTris * 3));
  if(triangleWriteResult.invalid())
  {
    return triangleWriteResult;
  }

  return result;
}

Result<> MeshingUtilities::RepairTriangleWindingExternal(INodeGeometry2D::SharedFaceList::store_type& triangles, const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel,
                                                         const IFilter::MessageHandler& mesgHandler)
{
  const usize componentCount = idsStore.getNumberOfComponents();
  if(componentCount > 2 || componentCount == 0)
  {
    return MakeErrorResult(
        -65770, fmt::format("MeshingUtilities::RepairTriangleWindingExternal: invalid ID array supplied. The ID array must have 1 or 2 components, supplied array components: {}.", componentCount));
  }

  const uint64 triangleCount = static_cast<uint64>(triangles.getNumberOfTuples());
  if(triangles.getNumberOfComponents() != 3 || idsStore.getNumberOfTuples() != triangles.getNumberOfTuples())
  {
    return MakeErrorResult(-65771, "MeshingUtilities::RepairTriangleWindingExternal: triangle and ID tuple shapes do not match.");
  }
  if(triangleCount == 0)
  {
    return {};
  }

  auto occurrenceResult = CreateWindingSort<VertexOccurrence>(CompareVertexOccurrences);
  if(occurrenceResult.invalid())
  {
    return ConvertResult(std::move(occurrenceResult));
  }
  auto contributionResult = CreateWindingSort<NeighborContribution>(CompareNeighborContributions);
  if(contributionResult.invalid())
  {
    return ConvertResult(std::move(contributionResult));
  }
  auto neighborResult = CreateWindingSort<WindingNeighbor>(CompareWindingNeighbors);
  if(neighborResult.invalid())
  {
    return ConvertResult(std::move(neighborResult));
  }
  auto seedResult = CreateWindingSort<FeatureSeed>(CompareFeatureSeeds);
  if(seedResult.invalid())
  {
    return ConvertResult(std::move(seedResult));
  }

  std::unique_ptr<IExternalSort> occurrences = std::move(occurrenceResult.value());
  std::unique_ptr<IExternalSort> contributions = std::move(contributionResult.value());
  std::unique_ptr<IExternalSort> neighbors = std::move(neighborResult.value());
  std::unique_ptr<IExternalSort> seeds = std::move(seedResult.value());
  WindingSortAppender<VertexOccurrence> occurrenceAppender(*occurrences, shouldCancel);
  WindingSortAppender<FeatureSeed> seedAppender(*seeds, shouldCancel);

  std::vector<IGeometry::MeshIndexType> faceBuffer;
  std::vector<int32> idBuffer;
  try
  {
    faceBuffer.resize(static_cast<usize>(k_WindingBatchRecords * 3));
    idBuffer.resize(static_cast<usize>(k_WindingBatchRecords * componentCount));
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-65791, "Triangle winding failed to allocate bounded face and ID scan buffers.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-65791, "Triangle winding failed to allocate bounded face and ID scan buffers.");
  }

  int32 maxFeature = 0;
  for(uint64 offset = 0; offset < triangleCount; offset += k_WindingBatchRecords)
  {
    if(shouldCancel)
    {
      return {};
    }
    const uint64 count = std::min<uint64>(k_WindingBatchRecords, triangleCount - offset);
    auto faceReadResult = triangles.copyIntoBuffer(static_cast<usize>(offset * 3), nonstd::span<IGeometry::MeshIndexType>(faceBuffer.data(), static_cast<usize>(count * 3)));
    if(faceReadResult.invalid())
    {
      return faceReadResult;
    }
    auto idReadResult = idsStore.copyIntoBuffer(static_cast<usize>(offset * componentCount), nonstd::span<int32>(idBuffer.data(), static_cast<usize>(count * componentCount)));
    if(idReadResult.invalid())
    {
      return idReadResult;
    }
    for(uint64 localTriangle = 0; localTriangle < count; ++localTriangle)
    {
      const uint64 triangle = offset + localTriangle;
      for(uint8 ordinal = 0; ordinal < 3; ++ordinal)
      {
        VertexOccurrence occurrence;
        occurrence.Vertex = faceBuffer[static_cast<usize>(localTriangle * 3 + ordinal)];
        occurrence.Triangle = triangle;
        occurrence.Ordinal = ordinal;
        auto appendResult = occurrenceAppender.append(occurrence);
        if(appendResult.invalid())
        {
          return appendResult;
        }
      }
      for(uint8 component = 0; component < componentCount; ++component)
      {
        const int32 feature = idBuffer[static_cast<usize>(localTriangle * componentCount + component)];
        maxFeature = std::max(maxFeature, feature);
        if(feature > 0)
        {
          FeatureSeed seed;
          seed.Feature = feature;
          seed.Triangle = triangle;
          seed.Component = component;
          auto appendResult = seedAppender.append(seed);
          if(appendResult.invalid())
          {
            return appendResult;
          }
        }
      }
    }
  }
  auto occurrenceFlushResult = occurrenceAppender.flush();
  if(occurrenceFlushResult.invalid())
  {
    return occurrenceFlushResult;
  }
  auto seedFlushResult = seedAppender.flush();
  if(seedFlushResult.invalid())
  {
    return seedFlushResult;
  }
  auto occurrenceFinishResult = FinishWindingSort(*occurrences, shouldCancel);
  if(occurrenceFinishResult.invalid())
  {
    return occurrenceFinishResult;
  }
  auto seedFinishResult = FinishWindingSort(*seeds, shouldCancel);
  if(seedFinishResult.invalid())
  {
    return seedFinishResult;
  }

  WindingSortReader<VertexOccurrence> occurrenceSourceReader(*occurrences, shouldCancel);
  WindingSortReader<VertexOccurrence> occurrenceCandidateReader(*occurrences, shouldCancel);
  WindingSortAppender<NeighborContribution> contributionAppender(*contributions, shouldCancel);
  uint64 occurrenceOffset = 0;
  while(occurrenceOffset < occurrences->recordCount())
  {
    if(shouldCancel)
    {
      return {};
    }
    auto firstResult = occurrenceSourceReader.read(occurrenceOffset);
    if(firstResult.invalid())
    {
      return ConvertResult(std::move(firstResult));
    }
    const IGeometry::MeshIndexType vertex = firstResult.value().Vertex;
    uint64 groupEnd = occurrenceOffset + 1;
    while(groupEnd < occurrences->recordCount())
    {
      auto nextResult = occurrenceSourceReader.read(groupEnd);
      if(nextResult.invalid())
      {
        return ConvertResult(std::move(nextResult));
      }
      if(nextResult.value().Vertex != vertex)
      {
        break;
      }
      ++groupEnd;
    }
    for(uint64 sourceOffset = occurrenceOffset; sourceOffset < groupEnd; ++sourceOffset)
    {
      auto sourceResult = occurrenceSourceReader.read(sourceOffset);
      if(sourceResult.invalid())
      {
        return ConvertResult(std::move(sourceResult));
      }
      const VertexOccurrence source = sourceResult.value();
      for(uint64 candidateOffset = occurrenceOffset; candidateOffset < groupEnd; ++candidateOffset)
      {
        auto candidateResult = occurrenceCandidateReader.read(candidateOffset);
        if(candidateResult.invalid())
        {
          return ConvertResult(std::move(candidateResult));
        }
        const VertexOccurrence candidate = candidateResult.value();
        if(candidate.Triangle == source.Triangle)
        {
          continue;
        }
        NeighborContribution contribution;
        contribution.Source = source.Triangle;
        contribution.Candidate = candidate.Triangle;
        contribution.SourceOrdinal = source.Ordinal;
        auto appendResult = contributionAppender.append(contribution);
        if(appendResult.invalid())
        {
          return appendResult;
        }
      }
    }
    occurrenceOffset = groupEnd;
  }
  auto contributionFlushResult = contributionAppender.flush();
  if(contributionFlushResult.invalid())
  {
    return contributionFlushResult;
  }
  auto contributionFinishResult = FinishWindingSort(*contributions, shouldCancel);
  if(contributionFinishResult.invalid())
  {
    return contributionFinishResult;
  }

  WindingSortReader<NeighborContribution> contributionReader(*contributions, shouldCancel);
  WindingSortAppender<WindingNeighbor> neighborAppender(*neighbors, shouldCancel);
  uint64 contributionOffset = 0;
  while(contributionOffset < contributions->recordCount())
  {
    if(shouldCancel)
    {
      return {};
    }
    auto firstResult = contributionReader.read(contributionOffset);
    if(firstResult.invalid())
    {
      return ConvertResult(std::move(firstResult));
    }
    const NeighborContribution first = firstResult.value();
    uint64 groupEnd = contributionOffset + 1;
    while(groupEnd < contributions->recordCount())
    {
      auto nextResult = contributionReader.read(groupEnd);
      if(nextResult.invalid())
      {
        return ConvertResult(std::move(nextResult));
      }
      if(nextResult.value().Source != first.Source || nextResult.value().Candidate != first.Candidate)
      {
        break;
      }
      ++groupEnd;
    }
    if(groupEnd - contributionOffset == 2)
    {
      WindingNeighbor neighbor;
      neighbor.Source = first.Source;
      neighbor.Candidate = first.Candidate;
      neighbor.SourceOrdinal = first.SourceOrdinal;
      auto appendResult = neighborAppender.append(neighbor);
      if(appendResult.invalid())
      {
        return appendResult;
      }
    }
    contributionOffset = groupEnd;
  }
  auto neighborFlushResult = neighborAppender.flush();
  if(neighborFlushResult.invalid())
  {
    return neighborFlushResult;
  }
  auto neighborFinishResult = FinishWindingSort(*neighbors, shouldCancel);
  if(neighborFinishResult.invalid())
  {
    return neighborFinishResult;
  }

  auto indexStoreResult = CreateWindingRecordStore<NeighborIndex>(triangleCount);
  if(indexStoreResult.invalid())
  {
    return ConvertResult(std::move(indexStoreResult));
  }
  auto stateStoreResult = CreateWindingRecordStore<TriangleWindingState>(triangleCount);
  if(stateStoreResult.invalid())
  {
    return ConvertResult(std::move(stateStoreResult));
  }
  const uint64 queueCapacity = std::max<uint64>(1, neighbors->recordCount());
  auto queueStoreResult = CreateWindingRecordStore<uint64>(queueCapacity);
  if(queueStoreResult.invalid())
  {
    return ConvertResult(std::move(queueStoreResult));
  }
  std::unique_ptr<ITemporaryRecordStore> indexStore = std::move(indexStoreResult.value());
  std::unique_ptr<ITemporaryRecordStore> stateStore = std::move(stateStoreResult.value());
  std::unique_ptr<ITemporaryRecordStore> queueStore = std::move(queueStoreResult.value());

  const NeighborIndex emptyIndex{};
  auto indexFillResult = indexStore->fill(0, triangleCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&emptyIndex), sizeof(emptyIndex)), shouldCancel);
  if(indexFillResult.invalid())
  {
    return indexFillResult;
  }
  const TriangleWindingState emptyState{};
  auto stateFillResult = stateStore->fill(0, triangleCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&emptyState), sizeof(emptyState)), shouldCancel);
  if(stateFillResult.invalid())
  {
    return stateFillResult;
  }
  const uint64 emptyQueueValue = 0;
  auto queueFillResult = queueStore->fill(0, queueCapacity, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&emptyQueueValue), sizeof(emptyQueueValue)), shouldCancel);
  if(queueFillResult.invalid())
  {
    return queueFillResult;
  }

  BoundedRecordPageCache<NeighborIndex> indexCache(*indexStore, k_WindingBatchRecords, k_WindingCachePages);
  BoundedRecordPageCache<TriangleWindingState> stateCache(*stateStore, k_WindingBatchRecords, k_WindingCachePages);
  BoundedRecordPageCache<uint64> queueCache(*queueStore, k_WindingBatchRecords, k_WindingCachePages);
  WindingSortReader<WindingNeighbor> neighborReader(*neighbors, shouldCancel);

  uint64 neighborOffset = 0;
  while(neighborOffset < neighbors->recordCount())
  {
    auto firstResult = neighborReader.read(neighborOffset);
    if(firstResult.invalid())
    {
      return ConvertResult(std::move(firstResult));
    }
    const uint64 source = firstResult.value().Source;
    if(source >= triangleCount)
    {
      return MakeErrorResult(-65792, "Triangle winding external adjacency contains an invalid source triangle.");
    }
    uint64 groupEnd = neighborOffset + 1;
    while(groupEnd < neighbors->recordCount())
    {
      auto nextResult = neighborReader.read(groupEnd);
      if(nextResult.invalid())
      {
        return ConvertResult(std::move(nextResult));
      }
      if(nextResult.value().Source != source)
      {
        break;
      }
      ++groupEnd;
    }
    auto writeResult = indexCache.write(source, NeighborIndex{neighborOffset, groupEnd - neighborOffset}, shouldCancel);
    if(writeResult.invalid())
    {
      return writeResult;
    }
    neighborOffset = groupEnd;
  }

  WindingDataStoreCache<IGeometry::MeshIndexType> faceCache(triangles, k_WindingBatchRecords, k_WindingCachePages);
  WindingDataStoreCache<int32> idCache(idsStore, k_WindingBatchRecords, k_WindingCachePages);
  WindingSortReader<FeatureSeed> seedReader(*seeds, shouldCancel);

  const auto readLabels = [&](uint64 triangle, std::array<int32, 2>& labels) -> Result<> { return idCache.readTuple(triangle, nonstd::span<int32>(labels.data(), componentCount)); };
  const auto belongsToFeature = [&](uint64 triangle, int32 feature) -> Result<bool> {
    std::array<int32, 2> labels = {0, 0};
    auto readResult = readLabels(triangle, labels);
    if(readResult.invalid())
    {
      return ConvertResultTo<bool>(std::move(readResult), false);
    }
    return {labels[0] == feature || (componentCount == 2 && labels[1] == feature)};
  };
  const auto readFace = [&](uint64 triangle, std::array<IGeometry::MeshIndexType, 3>& face) -> Result<> {
    return faceCache.readTuple(triangle, nonstd::span<IGeometry::MeshIndexType>(face.data(), face.size()));
  };

  uint64 unrepairedCount = 0;
  uint64 seedOffset = 0;
  auto progressStart = std::chrono::steady_clock::now();
  while(seedOffset < seeds->recordCount())
  {
    if(shouldCancel)
    {
      return faceCache.flush();
    }
    auto seedRecordResult = seedReader.read(seedOffset);
    if(seedRecordResult.invalid())
    {
      return ConvertResult(std::move(seedRecordResult));
    }
    const FeatureSeed seed = seedRecordResult.value();
    uint64 nextFeatureOffset = seedOffset + 1;
    while(nextFeatureOffset < seeds->recordCount())
    {
      auto nextResult = seedReader.read(nextFeatureOffset);
      if(nextResult.invalid())
      {
        return ConvertResult(std::move(nextResult));
      }
      if(nextResult.value().Feature != seed.Feature)
      {
        break;
      }
      ++nextFeatureOffset;
    }

    uint64 queueHead = 0;
    uint64 queueTail = 0;
    const auto enqueue = [&](uint64 triangle) -> Result<> {
      if(queueTail >= queueCapacity)
      {
        return MakeErrorResult(-65793, "Triangle winding bounded FIFO exceeded the external adjacency count.");
      }
      auto result = queueCache.write(queueTail, triangle, shouldCancel);
      if(result.valid())
      {
        ++queueTail;
      }
      return result;
    };
    const auto enqueueCompatibleNeighbors = [&](uint64 triangle) -> Result<> {
      auto indexResult = indexCache.read(triangle, shouldCancel);
      if(indexResult.invalid())
      {
        return ConvertResult(std::move(indexResult));
      }
      const NeighborIndex index = indexResult.value();
      for(uint64 localNeighbor = 0; localNeighbor < index.Count; ++localNeighbor)
      {
        auto neighborRecordResult = neighborReader.read(index.First + localNeighbor);
        if(neighborRecordResult.invalid())
        {
          return ConvertResult(std::move(neighborRecordResult));
        }
        const uint64 candidate = neighborRecordResult.value().Candidate;
        if(candidate >= triangleCount)
        {
          return MakeErrorResult(-65794, "Triangle winding external adjacency contains an invalid candidate triangle.");
        }
        auto compatibleResult = belongsToFeature(candidate, seed.Feature);
        if(compatibleResult.invalid())
        {
          return ConvertResult(std::move(compatibleResult));
        }
        if(compatibleResult.value())
        {
          auto enqueueResult = enqueue(candidate);
          if(enqueueResult.invalid())
          {
            return enqueueResult;
          }
        }
      }
      return {};
    };

    auto seedEnqueueResult = enqueueCompatibleNeighbors(seed.Triangle);
    if(seedEnqueueResult.invalid())
    {
      return seedEnqueueResult;
    }
    auto seedStateResult = stateCache.read(seed.Triangle, shouldCancel);
    if(seedStateResult.invalid())
    {
      return ConvertResult(std::move(seedStateResult));
    }
    TriangleWindingState seedState = seedStateResult.value();
    seedState.Visited = 1;
    auto seedStateWriteResult = stateCache.write(seed.Triangle, seedState, shouldCancel);
    if(seedStateWriteResult.invalid())
    {
      return seedStateWriteResult;
    }

    while(queueHead < queueTail)
    {
      if(shouldCancel)
      {
        return faceCache.flush();
      }
      auto queuedResult = queueCache.read(queueHead, shouldCancel);
      if(queuedResult.invalid())
      {
        return ConvertResult(std::move(queuedResult));
      }
      ++queueHead;
      const uint64 triangle = queuedResult.value();
      auto stateResult = stateCache.read(triangle, shouldCancel);
      if(stateResult.invalid())
      {
        return ConvertResult(std::move(stateResult));
      }
      TriangleWindingState state = stateResult.value();
      if(state.Visited != 0)
      {
        continue;
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - progressStart).count() > 1000)
      {
        mesgHandler(fmt::format("Current Feature: {}/{} | Progress : {:2.2f}%", seed.Feature, maxFeature, 100.0f * static_cast<float>(seed.Feature) / static_cast<float>(maxFeature + 1)));
        progressStart = std::chrono::steady_clock::now();
      }

      auto enqueueResult = enqueueCompatibleNeighbors(triangle);
      if(enqueueResult.invalid())
      {
        return enqueueResult;
      }
      state.Visited = 1;
      auto stateWriteResult = stateCache.write(triangle, state, shouldCancel);
      if(stateWriteResult.invalid())
      {
        return stateWriteResult;
      }

      std::array<IGeometry::MeshIndexType, 3> face = {0, 0, 0};
      auto faceReadResult = readFace(triangle, face);
      if(faceReadResult.invalid())
      {
        return faceReadResult;
      }
      bool conflict = false;
      auto indexResult = indexCache.read(triangle, shouldCancel);
      if(indexResult.invalid())
      {
        return ConvertResult(std::move(indexResult));
      }
      const NeighborIndex index = indexResult.value();
      for(uint64 localNeighbor = 0; localNeighbor < index.Count && !conflict; ++localNeighbor)
      {
        auto neighborRecordResult = neighborReader.read(index.First + localNeighbor);
        if(neighborRecordResult.invalid())
        {
          return ConvertResult(std::move(neighborRecordResult));
        }
        const uint64 candidate = neighborRecordResult.value().Candidate;
        auto compatibleResult = belongsToFeature(candidate, seed.Feature);
        if(compatibleResult.invalid())
        {
          return ConvertResult(std::move(compatibleResult));
        }
        if(!compatibleResult.value())
        {
          continue;
        }
        auto neighborStateResult = stateCache.read(candidate, shouldCancel);
        if(neighborStateResult.invalid())
        {
          return ConvertResult(std::move(neighborStateResult));
        }
        if(neighborStateResult.value().Visited == 0)
        {
          continue;
        }
        std::array<IGeometry::MeshIndexType, 3> neighborFace = {0, 0, 0};
        auto neighborFaceResult = readFace(candidate, neighborFace);
        if(neighborFaceResult.invalid())
        {
          return neighborFaceResult;
        }
        conflict = DirectedEdgesConflict(face, neighborFace, componentCount == 2 && neighborStateResult.value().Unmodified != 0);
      }

      if(conflict)
      {
        bool leaveUnmodified = false;
        if(componentCount == 2)
        {
          std::array<int32, 2> labels = {0, 0};
          auto labelReadResult = readLabels(triangle, labels);
          if(labelReadResult.invalid())
          {
            return labelReadResult;
          }
          const usize alternateOffset = labels[0] == seed.Feature ? 1 : 0;
          const int32 alternateLabel = labels[alternateOffset];
          leaveUnmodified = alternateLabel != 0 && alternateLabel < seed.Feature;
        }
        if(leaveUnmodified)
        {
          state.Unmodified = 1;
          ++unrepairedCount;
          auto unmodifiedWriteResult = stateCache.write(triangle, state, shouldCancel);
          if(unmodifiedWriteResult.invalid())
          {
            return unmodifiedWriteResult;
          }
        }
        else
        {
          std::swap(face[0], face[2]);
          auto faceWriteResult = faceCache.writeTuple(triangle, nonstd::span<const IGeometry::MeshIndexType>(face.data(), face.size()));
          if(faceWriteResult.invalid())
          {
            return faceWriteResult;
          }
        }
      }
    }
    seedOffset = nextFeatureOffset;
  }

  auto faceFlushResult = faceCache.flush();
  if(faceFlushResult.invalid())
  {
    return faceFlushResult;
  }
  if(unrepairedCount > 0)
  {
    return MakeWarningVoidResult(-56730, fmt::format("{} triangles cold not be made consistent, due to the nature of mesh implementation.", unrepairedCount));
  }
  return {};
}

MeshingUtilities::CalculateNormalsImpl::CalculateNormalsImpl(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts,
                                                             nx::core::Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel)
: m_Triangles(triangles)
, m_Vertices(verts)
, m_Normals(normals)
, m_ShouldCancel(shouldCancel)
{
}

void MeshingUtilities::CalculateNormalsImpl::generate(nx::core::types::usize start, nx::core::types::usize end) const
{
  for(usize triangle = start; triangle < end; triangle++)
  {
    if(m_ShouldCancel)
    {
      break;
    }

    const usize triangleIndex = triangle * 3;

    const usize vertAIndex = m_Triangles[triangleIndex] * 3;
    const Eigen::Vector3d vertA = Eigen::Vector3d{m_Vertices[vertAIndex], m_Vertices[vertAIndex + 1], m_Vertices[vertAIndex + 2]};
    const usize vertBIndex = m_Triangles[triangleIndex + 1] * 3;
    const Eigen::Vector3d vertB = Eigen::Vector3d{m_Vertices[vertBIndex], m_Vertices[vertBIndex + 1], m_Vertices[vertBIndex + 2]};
    const usize vertCIndex = m_Triangles[triangleIndex + 2] * 3;
    const Eigen::Vector3d vertC = Eigen::Vector3d{m_Vertices[vertCIndex], m_Vertices[vertCIndex + 1], m_Vertices[vertCIndex + 2]};

    const Eigen::Vector3d vecA = vertB - vertA;
    const Eigen::Vector3d vecB = vertC - vertA;

    Eigen::Vector3d normal = vecA.cross(vecB);
    normal.normalize();

    m_Normals[triangleIndex] = normal[0];
    m_Normals[triangleIndex + 1] = normal[1];
    m_Normals[triangleIndex + 2] = normal[2];
  }
}

void MeshingUtilities::CalculateNormalsImpl::operator()(const nx::core::Range& range) const
{
  generate(range.min(), range.max());
}

Result<> MeshingUtilities::MakeEmptyMeshWarning(const DataPath& triangleGeomPath, usize numCells, usize numVertices)
{
  return MakeWarningVoidResult(k_EmptyMeshAfterSkinRemovalWarning,
                               fmt::format("The 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode removed every face of geometry '{}'. All {} cells of the input have Feature Id 0 "
                                           "(background), so there is no internal interface and no Feature to cap. The Triangle Geometry now has 0 faces and {} vertices remaining.",
                                           triangleGeomPath.toString(), numCells, numVertices));
}

Result<> MeshingUtilities::MakeNoFacesPrunedWarning(const DataPath& triangleGeomPath)
{
  return MakeWarningVoidResult(k_NoFacesPrunedWarning,
                               fmt::format("The 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode removed 0 faces of geometry '{}': no bounding-box wall face is backed by "
                                           "background (Feature Id 0). This says nothing about the volume's interior -- a volume whose background is fully enclosed as interior porosity "
                                           "reaches this same warning, because none of that background borders a wall. There is nothing for this option to prune on this input; the output "
                                           "is identical to leaving it off.",
                                           triangleGeomPath.toString()));
}

Result<> MeshingUtilities::ValidateFeatureIdsAgainstSentinels(const Int32AbstractDataStore& featureIdsStore, const DataPath& featureIdsPath, bool rejectMaxInt32, const std::atomic_bool& shouldCancel,
                                                              const IFilter::MessageHandler& mesgHandler)
{
  const usize numTuples = featureIdsStore.getNumberOfTuples();
  mesgHandler(fmt::format("Validating {} Feature Ids against internal sentinel values...", numTuples));

  // Polled every k_CancelCheckInterval tuples rather than every tuple: at 512^3 (~134M tuples) this
  // loop is a full streaming pass under the out-of-core backend, and a per-tuple cancel check would
  // add overhead to what is otherwise a tight, uncontested read loop.
  constexpr usize k_CancelCheckInterval = 1'000'000;
  for(usize i = 0; i < numTuples; i++)
  {
    if(i % k_CancelCheckInterval == 0 && shouldCancel)
    {
      return {};
    }

    const int32 featureId = featureIdsStore[i];
    if(featureId < 0)
    {
      return MakeErrorResult(k_InvalidFeatureIdError, fmt::format("Feature Ids array '{}' contains a negative value ({}) at tuple index {}. This mesher reserves negative Feature Ids for an "
                                                                  "internal ghost/exterior sentinel convention; relabel this input so every Feature Id is >= 0. (Mitigation for the "
                                                                  "sentinel-collision design tracked separately as simplnx#1705.)",
                                                                  featureIdsPath.toString(), featureId, i));
    }
    if(rejectMaxInt32 && featureId == std::numeric_limits<int32>::max())
    {
      return MakeErrorResult(k_InvalidFeatureIdError, fmt::format("Feature Ids array '{}' contains the value {} at tuple index {}, which collides with an internal 'outside the volume' "
                                                                  "sentinel used by this mesher. Relabel this input so no Feature Id equals INT32_MAX. (Mitigation for the sentinel-collision "
                                                                  "design tracked separately as simplnx#1705.)",
                                                                  featureIdsPath.toString(), featureId, i));
    }
  }

  return {};
}
