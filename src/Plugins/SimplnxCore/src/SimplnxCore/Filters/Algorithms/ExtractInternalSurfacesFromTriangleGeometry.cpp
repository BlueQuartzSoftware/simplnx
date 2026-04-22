#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{

// -----------------------------------------------------------------------------
// Compact bookkeeping for the "which elements survive" question:
//   - vertNewIndex: dense per-vertex map (8 B * numVerts). New indices are assigned
//     in triangle-traversal order to preserve the contiguous-per-triangle invariant
//     relied on by downstream filters (triangle 0's three vertices land at new
//     indices 0..2 when all are newly-seen, etc.).
//   - triMask + triPrefixSum: 1-bit-per-triangle keep mask plus a sparse prefix-sum popcount
//     table sampled every k_PrefixSumGranularity bits. This replaces the legacy 8 B per
//     triangle dense map and delivers O(1)+small-popcount lookup of each kept triangle's
//     compact new index.
// The triangle-side savings compared to the legacy dense map are ~6.4x at the bit level
// plus the tiny prefix-sum table; vertex-side memory is unchanged vs legacy because the
// triangle-traversal ordering can't be recovered from a bitmap alone.
// -----------------------------------------------------------------------------
constexpr uint64 k_PrefixSumGranularity = 4096;
static_assert(k_PrefixSumGranularity % 64 == 0, "k_PrefixSumGranularity must be a multiple of 64");
constexpr uint64 k_WordsPerPrefixSum = k_PrefixSumGranularity / 64;

// Chunk size (in tuples) for streaming reads/writes. 65536 tuples keeps transient buffers
// under ~1 MB for typical element sizes while amortizing HDF5 chunk-op overhead.
constexpr usize k_ChunkTuples = 65536;

inline void bitmapSet(std::vector<uint64>& bitmap, uint64 bit)
{
  bitmap[bit >> 6] |= (1ULL << (bit & 63));
}

inline bool bitmapTest(const std::vector<uint64>& bitmap, uint64 bit)
{
  return (bitmap[bit >> 6] & (1ULL << (bit & 63))) != 0;
}

// Return popcount(bitmap[0..bit-1]), i.e. the new compact index assigned to the kept
// element at position `bit`.
inline uint64 remapIndex(uint64 bit, const std::vector<uint64>& bitmap, const std::vector<uint64>& prefixSum)
{
  const uint64 prefixSumIndex = bit / k_PrefixSumGranularity;
  uint64 result = prefixSum[prefixSumIndex];
  const uint64 startWord = prefixSumIndex * k_WordsPerPrefixSum;
  const uint64 bitWord = bit >> 6;
  for(uint64 w = startWord; w < bitWord; w++)
  {
    result += static_cast<uint64>(std::popcount(bitmap[w]));
  }
  const uint64 bitOffset = bit & 63;
  if(bitOffset != 0)
  {
    const uint64 partialMask = (1ULL << bitOffset) - 1;
    result += static_cast<uint64>(std::popcount(bitmap[bitWord] & partialMask));
  }
  return result;
}

// Build the prefix-sum popcount table for a completed bitmap. Returns total kept count
// (equivalent to popcount of the whole bitmap).
uint64 buildPrefixSumTable(const std::vector<uint64>& bitmap, std::vector<uint64>& prefixSum, uint64 numBits)
{
  const uint64 numPrefixSumEntries = (numBits + k_PrefixSumGranularity - 1) / k_PrefixSumGranularity;
  prefixSum.assign(numPrefixSumEntries, 0);
  uint64 running = 0;
  for(uint64 c = 0; c < numPrefixSumEntries; c++)
  {
    prefixSum[c] = running;
    const uint64 startWord = c * k_WordsPerPrefixSum;
    const uint64 endWord = std::min<uint64>(startWord + k_WordsPerPrefixSum, static_cast<uint64>(bitmap.size()));
    for(uint64 w = startWord; w < endWord; w++)
    {
      running += static_cast<uint64>(std::popcount(bitmap[w]));
    }
  }
  return running;
}

// Pass 1a: stream NodeTypes and mark vertices whose type is in [minType, maxType].
void buildVertOkMask(const Int8AbstractDataStore& nodeTypesStore, std::vector<uint64>& vertOkMask, int8 minType, int8 maxType, const std::atomic_bool& shouldCancel)
{
  const usize numVerts = nodeTypesStore.getNumberOfTuples();
  auto chunkBuf = std::make_unique<int8[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVerts; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return;
    }
    const usize count = std::min(k_ChunkTuples, numVerts - offset);
    nodeTypesStore.copyIntoBuffer(offset, nonstd::span<int8>(chunkBuf.get(), count));
    for(usize i = 0; i < count; i++)
    {
      const int8 nt = chunkBuf[i];
      if(nt >= minType && nt <= maxType)
      {
        bitmapSet(vertOkMask, offset + i);
      }
    }
  }
}

// Pass 1b: stream triangles, for each "all three vertices pass criterion" triangle:
//   - set its bit in triMask
//   - assign NEW-INDEX to any unseen vertex in vertNewIndex using triangle-traversal order
// The latter preserves the legacy behavior where triangle 0's freshly-seen vertices get
// new indices 0, 1, 2 in the order they're encountered within the triangle.
void scanTrianglesAndAssignVertexIndices(const UInt64AbstractDataStore& triangleStore, const std::vector<uint64>& vertOkMask, std::vector<uint64>& triMask,
                                         std::vector<IGeometry::MeshIndexType>& vertNewIndex, IGeometry::MeshIndexType& outNumKeptVerts, usize numTris, const std::atomic_bool& shouldCancel)
{
  using MeshIndexType = IGeometry::MeshIndexType;
  const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();
  MeshIndexType currentNewVertIndex = 0;

  auto chunkBuf = std::make_unique<uint64[]>(k_ChunkTuples * 3);
  for(usize offset = 0; offset < numTris; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      outNumKeptVerts = currentNewVertIndex;
      return;
    }
    const usize count = std::min(k_ChunkTuples, numTris - offset);
    triangleStore.copyIntoBuffer(offset * 3, nonstd::span<uint64>(chunkBuf.get(), count * 3));
    for(usize i = 0; i < count; i++)
    {
      const uint64 v0 = chunkBuf[i * 3 + 0];
      const uint64 v1 = chunkBuf[i * 3 + 1];
      const uint64 v2 = chunkBuf[i * 3 + 2];
      if(bitmapTest(vertOkMask, v0) && bitmapTest(vertOkMask, v1) && bitmapTest(vertOkMask, v2))
      {
        bitmapSet(triMask, offset + i);
        if(vertNewIndex[v0] == notSeen)
        {
          vertNewIndex[v0] = currentNewVertIndex++;
        }
        if(vertNewIndex[v1] == notSeen)
        {
          vertNewIndex[v1] = currentNewVertIndex++;
        }
        if(vertNewIndex[v2] == notSeen)
        {
          vertNewIndex[v2] = currentNewVertIndex++;
        }
      }
    }
  }
  outNumKeptVerts = currentNewVertIndex;
}

// Pass 3 / Pass 5 body: vertex-level array copy using the dense vertNewIndex map.
// Sources are bulk-read a chunk at a time; destinations are written one tuple at a
// time because the triangle-traversal new-index ordering is not monotonic in source
// order. This is still a strict improvement over the legacy operator[] loop, which
// issued one chunk read AND one chunk write per element.
struct VertexRemapCopyFunctor
{
  template <class T>
  void operator()(IDataArray* src, IDataArray* dst, const std::vector<IGeometry::MeshIndexType>& vertNewIndex, usize numInputTuples, const std::atomic_bool& shouldCancel) const
  {
    using MeshIndexType = IGeometry::MeshIndexType;
    const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();
    auto& srcStore = src->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& dstStore = dst->template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComps = srcStore.getNumberOfComponents();

    auto srcBuf = std::make_unique<T[]>(k_ChunkTuples * numComps);

    for(usize offset = 0; offset < numInputTuples; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize count = std::min(k_ChunkTuples, numInputTuples - offset);
      srcStore.copyIntoBuffer(offset * numComps, nonstd::span<T>(srcBuf.get(), count * numComps));
      for(usize i = 0; i < count; i++)
      {
        const MeshIndexType newIdx = vertNewIndex[offset + i];
        if(newIdx != notSeen)
        {
          // Per-tuple random write — one OOC chunk-op per kept vertex. Matches legacy cost
          // profile on the write side, but saves ~50% by bulk-reading the source.
          dstStore.copyFromBuffer(newIdx * numComps, nonstd::span<const T>(srcBuf.get() + i * numComps, numComps));
        }
      }
    }
  }
};

// Pass 4 body: copy kept triangles with their vertex indices rewritten to the new
// compact numbering. Because triMask+triPrefixSum assign new triangle indices sequentially
// in source order, both reads AND writes are bulk-chunked here.
void copyTrianglesRemapped(const UInt64AbstractDataStore& srcStore, UInt64AbstractDataStore& dstStore, const std::vector<uint64>& triMask, const std::vector<uint64>& triPrefixSum,
                           const std::vector<IGeometry::MeshIndexType>& vertNewIndex, usize numInputTris, const std::atomic_bool& shouldCancel)
{
  auto srcBuf = std::make_unique<uint64[]>(k_ChunkTuples * 3);
  auto dstBuf = std::make_unique<uint64[]>(k_ChunkTuples * 3);

  for(usize offset = 0; offset < numInputTris; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return;
    }
    const usize count = std::min(k_ChunkTuples, numInputTris - offset);
    srcStore.copyIntoBuffer(offset * 3, nonstd::span<uint64>(srcBuf.get(), count * 3));

    const uint64 dstStartNewTriIdx = remapIndex(offset, triMask, triPrefixSum);
    usize localKeptIdx = 0;
    for(usize i = 0; i < count; i++)
    {
      if(bitmapTest(triMask, offset + i))
      {
        dstBuf[localKeptIdx * 3 + 0] = vertNewIndex[srcBuf[i * 3 + 0]];
        dstBuf[localKeptIdx * 3 + 1] = vertNewIndex[srcBuf[i * 3 + 1]];
        dstBuf[localKeptIdx * 3 + 2] = vertNewIndex[srcBuf[i * 3 + 2]];
        localKeptIdx++;
      }
    }
    if(localKeptIdx > 0)
    {
      dstStore.copyFromBuffer(dstStartNewTriIdx * 3, nonstd::span<const uint64>(dstBuf.get(), localKeptIdx * 3));
    }
  }
}

// Pass 6 body: triangle-level attached-array copy. New triangle indices are monotonic
// in source order (via triMask+triPrefixSum) so both reads and writes are bulk-chunked.
struct TriangleAttachedCopyFunctor
{
  template <class T>
  void operator()(IDataArray* src, IDataArray* dst, const std::vector<uint64>& mask, const std::vector<uint64>& prefixSum, usize numInputTuples, const std::atomic_bool& shouldCancel) const
  {
    auto& srcStore = src->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& dstStore = dst->template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComps = srcStore.getNumberOfComponents();

    auto srcBuf = std::make_unique<T[]>(k_ChunkTuples * numComps);
    auto dstBuf = std::make_unique<T[]>(k_ChunkTuples * numComps);

    for(usize offset = 0; offset < numInputTuples; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return;
      }
      const usize count = std::min(k_ChunkTuples, numInputTuples - offset);
      srcStore.copyIntoBuffer(offset * numComps, nonstd::span<T>(srcBuf.get(), count * numComps));

      const uint64 dstStartNewIdx = remapIndex(offset, mask, prefixSum);
      usize localKeptIdx = 0;
      for(usize i = 0; i < count; i++)
      {
        if(bitmapTest(mask, offset + i))
        {
          for(usize c = 0; c < numComps; c++)
          {
            dstBuf[localKeptIdx * numComps + c] = srcBuf[i * numComps + c];
          }
          localKeptIdx++;
        }
      }
      if(localKeptIdx > 0)
      {
        dstStore.copyFromBuffer(dstStartNewIdx * numComps, nonstd::span<const T>(dstBuf.get(), localKeptIdx * numComps));
      }
    }
  }
};

} // namespace

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::ExtractInternalSurfacesFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                         ExtractInternalSurfacesFromTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractInternalSurfacesFromTriangleGeometry::~ExtractInternalSurfacesFromTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometry::operator()()
{
  auto internalTrianglesPath = m_InputValues->OutputTriangleGeometryPath;
  auto minMaxNodeValues = m_InputValues->NodeTypeRange;

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->InputTriangleGeometryPath);
  auto& internalTriangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(internalTrianglesPath);
  auto& vertices = *triangleGeom.getVertices();
  auto& triangles = *triangleGeom.getFaces();
  const usize numVerts = triangleGeom.getNumberOfVertices();
  const usize numTris = triangleGeom.getNumberOfFaces();

  auto& nodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesPath);

  auto internalVerticesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedVertexListName);
  internalTriangleGeom.setVertices(*m_DataStructure.getDataAs<Float32Array>(internalVerticesPath));

  auto internalFacesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedFacesListName);
  internalTriangleGeom.setFaceList(*m_DataStructure.getDataAs<UInt64Array>(internalFacesPath));

  const auto& trianglesStore = triangles.getDataStoreRef();
  const auto& nodeTypesStore = nodeTypes.getDataStoreRef();

  using MeshIndexType = IGeometry::MeshIndexType;
  const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();

  // Pass 1a — stream NodeTypes once to build a per-vertex "passes criterion" bitmap.
  // Transient-only; freed after Pass 1b consumes it.
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Scanning NodeTypes..."});
  std::vector<uint64> vertOkMask((numVerts + 63) / 64, 0ULL);
  buildVertOkMask(nodeTypesStore, vertOkMask, minMaxNodeValues[0], minMaxNodeValues[1], m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }

  // Pass 1b — stream triangles: for each "all three vertices ok" triangle set its triMask
  // bit AND assign new indices to its unseen vertices in triangle-traversal order. This
  // matches the legacy filter's ordering invariant.
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Scanning triangles..."});
  std::vector<uint64> triMask((numTris + 63) / 64, 0ULL);
  std::vector<MeshIndexType> vertNewIndex(numVerts, notSeen);
  MeshIndexType numKeptVerts = 0;
  scanTrianglesAndAssignVertexIndices(trianglesStore, vertOkMask, triMask, vertNewIndex, numKeptVerts, numTris, m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }

  // vertOkMask only needed during Pass 1b — release its RAM.
  vertOkMask.clear();
  vertOkMask.shrink_to_fit();

  // Pass 2 — build the triangle prefix-sum table (sparse, O(numTris / k_PrefixSumGranularity)).
  std::vector<uint64> triPrefixSum;
  const uint64 numKeptTris = buildPrefixSumTable(triMask, triPrefixSum, numTris);

  // Resize the output geometry and attribute matrices to the compact kept counts.
  internalTriangleGeom.resizeVertexList(numKeptVerts);
  internalTriangleGeom.resizeFaceList(numKeptTris);
  internalTriangleGeom.getVertexAttributeMatrix()->resizeTuples({numKeptVerts});
  internalTriangleGeom.getFaceAttributeMatrix()->resizeTuples({numKeptTris});

  IGeometry::SharedVertexList* internalVerts = internalTriangleGeom.getVertices();
  IGeometry::SharedFaceList* internalTriangles = internalTriangleGeom.getFaces();

  // Pass 3 — copy kept vertex XYZ coordinates into the compact output.
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Copying vertices..."});
  VertexRemapCopyFunctor{}.operator()<float32>(&vertices, internalVerts, vertNewIndex, numVerts, m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }

  // Pass 4 — copy kept triangles with vertex indices remapped.
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, "Copying triangles..."});
  copyTrianglesRemapped(trianglesStore, internalTriangles->getDataStoreRef(), triMask, triPrefixSum, vertNewIndex, numTris, m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }

  // Pass 5 — copy per-vertex attached arrays using the dense vertex map.
  for(const auto& targetArrayPath : m_InputValues->CopyVertexArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->VertexAttributeMatrixName).createChildPath(targetArrayPath.getTargetName());
    auto* src = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* dest = m_DataStructure.getDataAs<IDataArray>(destinationPath);
    ExecuteDataFunction(VertexRemapCopyFunctor{}, src->getDataType(), src, dest, vertNewIndex, numVerts, m_ShouldCancel);
  }

  // Pass 6 — copy per-triangle attached arrays using the triangle mask + prefix sum.
  for(const auto& targetArrayPath : m_InputValues->CopyTriangleArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->TriangleAttributeMatrixName).createChildPath(targetArrayPath.getTargetName());
    auto* src = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* dest = m_DataStructure.getDataAs<IDataArray>(destinationPath);
    dest->resizeTuples({numKeptTris});
    ExecuteDataFunction(TriangleAttachedCopyFunctor{}, src->getDataType(), src, dest, triMask, triPrefixSum, numTris, m_ShouldCancel);
  }

  return {};
}
