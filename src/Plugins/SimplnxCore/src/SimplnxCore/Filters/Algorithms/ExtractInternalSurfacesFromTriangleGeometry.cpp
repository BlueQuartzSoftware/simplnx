#include "ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <nonstd/span.hpp>

#include <bit>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{

// Vertex remapping stays dense to preserve first triangle-encounter order.
// Triangle remapping uses a bitmap and a sparse popcount prefix table.
constexpr uint64 k_PrefixSumGranularity = 4096;
static_assert(k_PrefixSumGranularity % 64 == 0, "k_PrefixSumGranularity must be a multiple of 64");
constexpr uint64 k_WordsPerPrefixSum = k_PrefixSumGranularity / 64;

// Fixed tuple chunks amortize store calls without volume-sized transfer buffers.
constexpr usize k_ChunkTuples = 65536;

inline void BitmapSet(std::vector<uint64>& bitmap, uint64 bit)
{
  bitmap[bit >> 6] |= (1ULL << (bit & 63));
}

inline bool BitmapTest(const std::vector<uint64>& bitmap, uint64 bit)
{
  return (bitmap[bit >> 6] & (1ULL << (bit & 63))) != 0;
}

// Count set bits before bit to find its compact output index.
inline uint64 RemapIndex(uint64 bit, const std::vector<uint64>& bitmap, const std::vector<uint64>& prefixSum)
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

// Build sparse cumulative popcounts and return the total set-bit count.
uint64 BuildPrefixSumTable(const std::vector<uint64>& bitmap, std::vector<uint64>& prefixSum, uint64 numBits)
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

/**
 * @brief Marks vertices whose node type is in the inclusive range.
 * @param nodeTypesStore Supplies one type per source vertex.
 * @param vertOkMask Receives accepted-vertex bits.
 * @param minType Inclusive minimum accepted node type.
 * @param maxType Inclusive maximum accepted node type.
 * @param shouldCancel Stops before a later input chunk.
 * @param reportProgress Reports completed source tuples, when supplied.
 * @return The first node-type bulk-read error.
 */
Result<> BuildVertOkMask(const Int8AbstractDataStoreType& nodeTypesStore, std::vector<uint64>& vertOkMask, int8 minType, int8 maxType, const std::atomic_bool& shouldCancel,
                         const std::function<void(usize)>& reportProgress = {})
{
  const usize numVerts = nodeTypesStore.getNumberOfTuples();
  auto chunkBuf = std::make_unique<int8[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVerts; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, numVerts - offset);
    Result<> readResult = nodeTypesStore.copyIntoBuffer(offset, nonstd::span<int8>(chunkBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      const int8 nt = chunkBuf[i];
      if(nt >= minType && nt <= maxType)
      {
        BitmapSet(vertOkMask, offset + i);
      }
    }
    if(reportProgress)
    {
      reportProgress(offset + count);
    }
  }
  return {};
}

/**
 * @brief Keeps triangles with three accepted vertices and assigns compact vertex indices.
 * @param triangleStore Supplies source triangle connectivity.
 * @param vertOkMask Selects accepted source vertices.
 * @param triMask Receives kept-triangle bits.
 * @param vertNewIndex Receives source-to-output vertex indices.
 * @param outNumKeptVerts Receives the number of output vertices.
 * @param numTris Number of source triangles.
 * @param shouldCancel Stops before a later input chunk.
 * @param reportProgress Reports completed source tuples, when supplied.
 * @return The first triangle bulk-read error.
 *
 * First encounter order preserves triangle traversal order.
 */
Result<> ScanTrianglesAndAssignVertexIndices(const UInt64AbstractDataStoreType& triangleStore, const std::vector<uint64>& vertOkMask, std::vector<uint64>& triMask,
                                             std::vector<IGeometry::MeshIndexType>& vertNewIndex, IGeometry::MeshIndexType& outNumKeptVerts, usize numTris, const std::atomic_bool& shouldCancel,
                                             const std::function<void(usize)>& reportProgress = {})
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
      return {};
    }
    const usize count = std::min(k_ChunkTuples, numTris - offset);
    Result<> readResult = triangleStore.copyIntoBuffer(offset * 3, nonstd::span<uint64>(chunkBuf.get(), count * 3));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      const uint64 v0 = chunkBuf[i * 3 + 0];
      const uint64 v1 = chunkBuf[i * 3 + 1];
      const uint64 v2 = chunkBuf[i * 3 + 2];
      if(BitmapTest(vertOkMask, v0) && BitmapTest(vertOkMask, v1) && BitmapTest(vertOkMask, v2))
      {
        BitmapSet(triMask, offset + i);
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
    if(reportProgress)
    {
      reportProgress(offset + count);
    }
  }
  outNumKeptVerts = currentNewVertIndex;
  return {};
}

/**
 * @brief Keeps internal triangles selected by their face labels.
 * @param triangleStore Supplies source triangle connectivity.
 * @param faceLabelsStore Supplies two labels per source triangle.
 * @param triMask Receives kept-triangle bits.
 * @param vertNewIndex Receives source-to-output vertex indices.
 * @param outNumKeptVerts Receives the number of output vertices.
 * @param numTris Number of source triangles.
 * @param shouldCancel Stops before a later input chunk.
 * @param reportProgress Reports completed source tuples, when supplied.
 * @return The first connectivity or face-label bulk-read error.
 */
Result<> ScanTrianglesByFaceLabels(const UInt64AbstractDataStoreType& triangleStore, const Int32AbstractDataStoreType& faceLabelsStore, std::vector<uint64>& triMask,
                                   std::vector<IGeometry::MeshIndexType>& vertNewIndex, IGeometry::MeshIndexType& outNumKeptVerts, usize numTris, const std::atomic_bool& shouldCancel,
                                   const std::function<void(usize)>& reportProgress = {})
{
  using MeshIndexType = IGeometry::MeshIndexType;
  const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();
  MeshIndexType currentNewVertIndex = 0;
  auto triangleBuffer = std::make_unique<uint64[]>(k_ChunkTuples * 3);
  auto faceLabelBuffer = std::make_unique<int32[]>(k_ChunkTuples * 2);

  for(usize offset = 0; offset < numTris; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      outNumKeptVerts = currentNewVertIndex;
      return {};
    }
    const usize count = std::min(k_ChunkTuples, numTris - offset);
    Result<> readResult = triangleStore.copyIntoBuffer(offset * 3, nonstd::span<uint64>(triangleBuffer.get(), count * 3));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = faceLabelsStore.copyIntoBuffer(offset * 2, nonstd::span<int32>(faceLabelBuffer.get(), count * 2));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      const int32 labelA = faceLabelBuffer[i * 2];
      const int32 labelB = faceLabelBuffer[i * 2 + 1];
      if((labelA == -1 && labelB == 0) || (labelB == -1 && labelA == 0))
      {
        continue;
      }

      BitmapSet(triMask, offset + i);
      for(usize component = 0; component < 3; component++)
      {
        const uint64 vertexIndex = triangleBuffer[i * 3 + component];
        if(vertNewIndex[vertexIndex] == notSeen)
        {
          vertNewIndex[vertexIndex] = currentNewVertIndex++;
        }
      }
    }
    if(reportProgress)
    {
      reportProgress(offset + count);
    }
  }
  outNumKeptVerts = currentNewVertIndex;
  return {};
}

// Nonmonotonic output order requires one destination tuple write per kept vertex.
// Chunked source reads keep memory bounded.
struct VertexRemapCopyFunctor
{
  /**
   * @brief Copies one typed vertex array.
   * @tparam T Specifies the array value type.
   * @param src Supplies source vertex tuples.
   * @param dst Receives compacted vertex tuples.
   * @param vertNewIndex Maps source vertices to output vertices.
   * @param numInputTuples Number of source tuples.
   * @param shouldCancel Stops before a later input chunk.
   * @param reportProgress Reports completed source tuples, when supplied.
   * @return The first source or destination bulk-I/O error.
   */
  template <class T>
  Result<> operator()(IDataArray* src, IDataArray* dst, const std::vector<IGeometry::MeshIndexType>& vertNewIndex, usize numInputTuples, const std::atomic_bool& shouldCancel,
                      const std::function<void(usize)>& reportProgress = {}) const
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
        return {};
      }
      const usize count = std::min(k_ChunkTuples, numInputTuples - offset);
      Result<> ioResult = srcStore.copyIntoBuffer(offset * numComps, nonstd::span<T>(srcBuf.get(), count * numComps));
      if(ioResult.invalid())
      {
        return ioResult;
      }
      for(usize i = 0; i < count; i++)
      {
        const MeshIndexType newIdx = vertNewIndex[offset + i];
        if(newIdx != notSeen)
        {
          // Output order requires one possibly random store write per kept vertex.
          ioResult = dstStore.copyFromBuffer(newIdx * numComps, nonstd::span<const T>(srcBuf.get() + i * numComps, numComps));
          if(ioResult.invalid())
          {
            return ioResult;
          }
        }
      }
      if(reportProgress)
      {
        reportProgress(offset + count);
      }
    }
    return {};
  }
};

/**
 * @brief Copies triangles in source order and replaces vertex indices with compact values.
 * @param srcStore Supplies source triangle connectivity.
 * @param dstStore Receives compacted triangle connectivity.
 * @param triMask Selects source triangles.
 * @param triPrefixSum Maps source triangle positions to output positions.
 * @param vertNewIndex Maps source vertices to output vertices.
 * @param numInputTris Number of source triangles.
 * @param shouldCancel Stops before a later input chunk.
 * @param reportProgress Reports completed source tuples, when supplied.
 * @return The first connectivity bulk-I/O error.
 */
Result<> CopyTrianglesRemapped(const UInt64AbstractDataStoreType& srcStore, UInt64AbstractDataStoreType& dstStore, const std::vector<uint64>& triMask, const std::vector<uint64>& triPrefixSum,
                               const std::vector<IGeometry::MeshIndexType>& vertNewIndex, usize numInputTris, const std::atomic_bool& shouldCancel,
                               const std::function<void(usize)>& reportProgress = {})
{
  auto srcBuf = std::make_unique<uint64[]>(k_ChunkTuples * 3);
  auto dstBuf = std::make_unique<uint64[]>(k_ChunkTuples * 3);

  for(usize offset = 0; offset < numInputTris; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, numInputTris - offset);
    Result<> ioResult = srcStore.copyIntoBuffer(offset * 3, nonstd::span<uint64>(srcBuf.get(), count * 3));
    if(ioResult.invalid())
    {
      return ioResult;
    }

    const uint64 dstStartNewTriIdx = RemapIndex(offset, triMask, triPrefixSum);
    usize localKeptIdx = 0;
    for(usize i = 0; i < count; i++)
    {
      if(BitmapTest(triMask, offset + i))
      {
        dstBuf[localKeptIdx * 3 + 0] = vertNewIndex[srcBuf[i * 3 + 0]];
        dstBuf[localKeptIdx * 3 + 1] = vertNewIndex[srcBuf[i * 3 + 1]];
        dstBuf[localKeptIdx * 3 + 2] = vertNewIndex[srcBuf[i * 3 + 2]];
        localKeptIdx++;
      }
    }
    if(localKeptIdx > 0)
    {
      ioResult = dstStore.copyFromBuffer(dstStartNewTriIdx * 3, nonstd::span<const uint64>(dstBuf.get(), localKeptIdx * 3));
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }
    if(reportProgress)
    {
      reportProgress(offset + count);
    }
  }
  return {};
}

/**
 * @struct TriangleAttachedCopyFunctor
 * @brief Copies selected triangle tuples in monotonic compact order.
 *
 * Source and output transfers use fixed chunks.
 */
struct TriangleAttachedCopyFunctor
{
  /**
   * @brief Copies one typed triangle array.
   * @tparam T Specifies the array value type.
   * @param src Supplies source triangle tuples.
   * @param dst Receives compacted triangle tuples.
   * @param mask Selects source triangles.
   * @param prefixSum Maps source triangle positions to output positions.
   * @param numInputTuples Number of source tuples.
   * @param shouldCancel Stops before a later input chunk.
   * @return The first source or destination bulk-I/O error.
   */
  template <class T>
  Result<> operator()(IDataArray* src, IDataArray* dst, const std::vector<uint64>& mask, const std::vector<uint64>& prefixSum, usize numInputTuples, const std::atomic_bool& shouldCancel) const
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
        return {};
      }
      const usize count = std::min(k_ChunkTuples, numInputTuples - offset);
      Result<> ioResult = srcStore.copyIntoBuffer(offset * numComps, nonstd::span<T>(srcBuf.get(), count * numComps));
      if(ioResult.invalid())
      {
        return ioResult;
      }

      const uint64 dstStartNewIdx = RemapIndex(offset, mask, prefixSum);
      usize localKeptIdx = 0;
      for(usize i = 0; i < count; i++)
      {
        if(BitmapTest(mask, offset + i))
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
        ioResult = dstStore.copyFromBuffer(dstStartNewIdx * numComps, nonstd::span<const T>(dstBuf.get(), localKeptIdx * numComps));
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }
    return {};
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

// -----------------------------------------------------------------------------
Result<> ExtractInternalSurfacesFromTriangleGeometry::operator()()
{
  auto internalTrianglesPath = m_InputValues->output_triangle_geometry_path;
  auto minMaxNodeValues = m_InputValues->node_type_range;

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->input_triangle_geometry_path);
  auto& internalTriangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(internalTrianglesPath);
  auto& vertices = *triangleGeom.getVertices();
  auto& triangles = *triangleGeom.getFaces();
  const usize numVerts = triangleGeom.getNumberOfVertices();
  const usize numTris = triangleGeom.getNumberOfFaces();

  auto internalVerticesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedVertexListName);
  internalTriangleGeom.setVertices(*m_DataStructure.getDataAs<Float32ArrayType>(internalVerticesPath));

  auto internalFacesPath = internalTrianglesPath.createChildPath(TriangleGeom::k_SharedFacesListName);
  internalTriangleGeom.setFaceList(*m_DataStructure.getDataAs<UInt64ArrayType>(internalFacesPath));

  const auto& trianglesStore = triangles.getDataStoreRef();

  using MeshIndexType = IGeometry::MeshIndexType;
  const MeshIndexType notSeen = std::numeric_limits<MeshIndexType>::max();

  std::vector<uint64> triMask((numTris + 63) / 64, 0ULL);
  std::vector<MeshIndexType> vertNewIndex(numVerts, notSeen);
  MeshIndexType numKeptVerts = 0;

  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  const auto reportProgress = [&progressThrottle](usize completed) { progressThrottle.updatePercent(completed); };

  if(m_InputValues->criterion_mode == 0)
  {
    const auto& nodeTypesStore = m_DataStructure.getDataRefAs<Int8ArrayType>(m_InputValues->node_types_path).getDataStoreRef();
    std::vector<uint64> vertOkMask((numVerts + 63) / 64, 0ULL);
    m_MessageHandler.sendInfoMessage("Selecting Mesh Vertices");
    progressThrottle.reset(numVerts, "Selecting Mesh Vertices");
    Result<> operationResult = BuildVertOkMask(nodeTypesStore, vertOkMask, minMaxNodeValues[0], minMaxNodeValues[1], m_ShouldCancel, reportProgress);
    if(operationResult.invalid())
    {
      return operationResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    m_MessageHandler.sendInfoMessage("Selecting Mesh Triangles");
    progressThrottle.reset(numTris, "Selecting Mesh Triangles");
    operationResult = ScanTrianglesAndAssignVertexIndices(trianglesStore, vertOkMask, triMask, vertNewIndex, numKeptVerts, numTris, m_ShouldCancel, reportProgress);
    if(operationResult.invalid())
    {
      return operationResult;
    }
  }
  else
  {
    const auto& faceLabelsStore = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->face_labels_path).getDataStoreRef();
    m_MessageHandler.sendInfoMessage("Selecting Mesh Triangles");
    progressThrottle.reset(numTris, "Selecting Mesh Triangles");
    Result<> operationResult = ScanTrianglesByFaceLabels(trianglesStore, faceLabelsStore, triMask, vertNewIndex, numKeptVerts, numTris, m_ShouldCancel, reportProgress);
    if(operationResult.invalid())
    {
      return operationResult;
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Pass 2 — build the triangle prefix-sum table (sparse, O(numTris / k_PrefixSumGranularity)).
  m_MessageHandler.sendInfoMessage("Building Triangle Index Map");
  std::vector<uint64> triPrefixSum;
  const uint64 numKeptTris = BuildPrefixSumTable(triMask, triPrefixSum, numTris);

  // Resize the output geometry and attribute matrices to the compact kept counts.
  Result<> operationResult = internalTriangleGeom.resizeVertexList(numKeptVerts);
  if(operationResult.invalid())
  {
    return operationResult;
  }
  operationResult = internalTriangleGeom.resizeFaceList(numKeptTris);
  if(operationResult.invalid())
  {
    return operationResult;
  }
  operationResult = internalTriangleGeom.getVertexAttributeMatrix()->resizeTuples({numKeptVerts});
  if(operationResult.invalid())
  {
    return operationResult;
  }
  operationResult = internalTriangleGeom.getFaceAttributeMatrix()->resizeTuples({numKeptTris});
  if(operationResult.invalid())
  {
    return operationResult;
  }

  IGeometry::SharedVertexListType* internalVertsPtr = internalTriangleGeom.getVertices();
  IGeometry::SharedFaceListType* internalTrianglesPtr = internalTriangleGeom.getFaces();

  // Pass 3 — copy kept vertex XYZ coordinates into the compact output.
  m_MessageHandler.sendInfoMessage("Copying Mesh Vertices");
  progressThrottle.reset(numVerts, "Copying Mesh Vertices");
  operationResult = VertexRemapCopyFunctor{}.operator()<float32>(&vertices, internalVertsPtr, vertNewIndex, numVerts, m_ShouldCancel, reportProgress);
  if(operationResult.invalid())
  {
    return operationResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Pass 4 — copy kept triangles with vertex indices remapped.
  m_MessageHandler.sendInfoMessage("Copying Mesh Triangles");
  progressThrottle.reset(numTris, "Copying Mesh Triangles");
  operationResult = CopyTrianglesRemapped(trianglesStore, internalTrianglesPtr->getDataStoreRef(), triMask, triPrefixSum, vertNewIndex, numTris, m_ShouldCancel, reportProgress);
  if(operationResult.invalid())
  {
    return operationResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  m_MessageHandler.sendInfoMessage("Copying Attached Arrays");
  progressThrottle.reset(m_InputValues->copy_vertex_array_paths.size() + m_InputValues->copy_triangle_array_paths.size(), "Copying Attached Arrays");
  usize copiedArrays = 0;

  // Pass 5 — copy per-vertex attached arrays using the dense vertex map.
  for(const auto& targetArrayPath : m_InputValues->copy_vertex_array_paths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->vertex_attribute_matrix_name).createChildPath(targetArrayPath.getTargetName());
    auto* srcPtr = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* destPtr = m_DataStructure.getDataAs<IDataArray>(destinationPath);
    operationResult = ExecuteDataFunction(VertexRemapCopyFunctor{}, srcPtr->getDataType(), srcPtr, destPtr, vertNewIndex, numVerts, m_ShouldCancel, std::function<void(usize)>{});
    if(operationResult.invalid())
    {
      return operationResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    progressThrottle.updateCount(++copiedArrays);
  }

  // Pass 6 — copy per-triangle attached arrays using the triangle mask + prefix sum.
  for(const auto& targetArrayPath : m_InputValues->copy_triangle_array_paths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    DataPath destinationPath = internalTrianglesPath.createChildPath(m_InputValues->triangle_attribute_matrix_name).createChildPath(targetArrayPath.getTargetName());
    auto* srcPtr = m_DataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* destPtr = m_DataStructure.getDataAs<IDataArray>(destinationPath);
    operationResult = destPtr->resizeTuples({numKeptTris});
    if(operationResult.invalid())
    {
      return operationResult;
    }
    operationResult = ExecuteDataFunction(TriangleAttachedCopyFunctor{}, srcPtr->getDataType(), srcPtr, destPtr, triMask, triPrefixSum, numTris, m_ShouldCancel);
    if(operationResult.invalid())
    {
      return operationResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    progressThrottle.updateCount(++copiedArrays);
  }

  return {};
}
