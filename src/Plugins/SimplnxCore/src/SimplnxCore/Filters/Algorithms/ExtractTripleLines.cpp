#include "ExtractTripleLines.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// Each half of an edge key holds one 32-bit source vertex index.
constexpr uint64 k_MaxVertexCount = std::numeric_limits<uint32>::max();
constexpr uint64 k_VertexIndexMask = k_MaxVertexCount;
constexpr int32 k_EmptyLabel = std::numeric_limits<int32>::min();

/**
 * @brief Stores a bounded number of unique normalized Feature Ids without per-Feature allocations.
 * @tparam Capacity Limits the retained Feature Id count.
 */
template <usize Capacity>
struct FeatureSet
{
  std::array<int32, Capacity> Features;

  FeatureSet()
  {
    Features.fill(k_EmptyLabel);
  }

  void insert(int32 featureId)
  {
    for(auto& storedLabel : Features)
    {
      if(storedLabel == featureId)
      {
        return;
      }
      if(storedLabel == k_EmptyLabel)
      {
        storedLabel = featureId;
        return;
      }
    }
  }

  uint8 count() const
  {
    return static_cast<uint8>(std::count_if(Features.begin(), Features.end(), [](int32 label) { return label != k_EmptyLabel; }));
  }
};
static_assert(sizeof(FeatureSet<3>) == 12);

constexpr int32 NormalizeLabel(int32 label)
{
  return label < 0 ? -1 : label;
}

constexpr uint64 MakeEdgeKey(uint64 vertex0, uint64 vertex1)
{
  return (vertex0 < vertex1) ? ((vertex0 << 32) | vertex1) : ((vertex1 << 32) | vertex0);
}

// -----------------------------------------------------------------------------
/**
 * @brief Copies sorted source vertices through one retained read page and one output block.
 * @param source Provides source XYZ tuples.
 * @param destination Receives compact XYZ tuples.
 * @param indices Supplies ascending, unique source vertex indices.
 * @param sourceNodes Supplies optional Node Types paired with destinationNodes.
 * @param destinationNodes Receives optional compact Node Types.
 * @param shouldCancel Signals cancellation between transfers and gathered vertices.
 * @param progress Receives completed block progress.
 * @return The first transfer error, admission/allocation error, or accumulated warnings.
 * @pre Indices refer to source tuples. Node Types pointers are paired and have compatible extents.
 */
Result<> copyTripleLineVertices(const Float32AbstractDataStore& source, Float32AbstractDataStore& destination, nonstd::span<const uint64> indices, const Int8AbstractDataStore* sourceNodes,
                                Int8AbstractDataStore* destinationNodes, const std::atomic_bool& shouldCancel, ThrottledMessageHandler& progress)
{
  if(indices.empty() || shouldCancel)
  {
    return {};
  }
  const bool copyNodes = sourceNodes != nullptr;
  const usize bytesPerVertex = (6 * sizeof(float32)) + (copyNodes ? 2 * sizeof(int8) : 0);
  const usize maximum = std::numeric_limits<usize>::max();
  if(indices.size() > maximum / bytesPerVertex || source.getNumberOfTuples() > maximum / 3)
  {
    return MakeErrorResult(-57407, "Triple line vertex transfer sizes exceed addressable memory. Reduce the mesh size before extracting triple lines.");
  }
  const usize requestedBytes = indices.size() * bytesPerVertex;
  auto reservation = CacheMemoryBudgetManager::instance().reserveWorkingMemory(requestedBytes);
  const usize capacity = static_cast<usize>(std::min<uint64>(reservation.sizeBytes() / bytesPerVertex, indices.size()));
  if(capacity == 0)
  {
    return MakeErrorResult(-57408, fmt::format("Triple line vertex copying requested {} working bytes and received {}. At least {} bytes are required. Increase the cache memory budget or finish "
                                               "other processing before retrying.",
                                               requestedBytes, reservation.sizeBytes(), bytesPerVertex));
  }
  reservation.shrinkTo(capacity * bytesPerVertex);
  Result<> result;
  try
  {
    auto page = std::make_unique<std::vector<float32>>(capacity * 3);
    auto output = std::make_unique<std::vector<float32>>(capacity * 3);
    auto nodePage = copyNodes ? std::make_unique<std::vector<int8>>(capacity) : nullptr;
    auto nodeOutput = copyNodes ? std::make_unique<std::vector<int8>>(capacity) : nullptr;
    usize pageBegin = 0;
    usize pageEnd = 0;
    for(usize begin = 0; begin < indices.size();)
    {
      const usize count = std::min(capacity, indices.size() - begin);
      for(usize local = 0; local < count; ++local)
      {
        if(shouldCancel)
        {
          return result;
        }
        const usize original = static_cast<usize>(indices[begin + local]);
        if(original >= pageEnd)
        {
          pageBegin = original;
          const usize pageCount = std::min(capacity, source.getNumberOfTuples() - pageBegin);
          pageEnd = pageBegin + pageCount;
          // Keep accumulated diagnostics until both the transfer and its merge succeed.
          auto readResult = source.copyIntoBuffer(pageBegin * 3, {page->data(), pageCount * 3});
          result = MergeResults(result, std::move(readResult));
          if(result.invalid() || shouldCancel)
          {
            return result;
          }
          if(copyNodes)
          {
            auto nodeReadResult = sourceNodes->copyIntoBuffer(pageBegin, {nodePage->data(), pageCount});
            result = MergeResults(result, std::move(nodeReadResult));
            if(result.invalid() || shouldCancel)
            {
              return result;
            }
          }
        }
        const usize pageIndex = original - pageBegin;
        for(usize component = 0; component < 3; ++component)
        {
          (*output)[(local * 3) + component] = (*page)[(pageIndex * 3) + component];
        }
        if(copyNodes)
        {
          (*nodeOutput)[local] = (*nodePage)[pageIndex];
        }
      }
      if(shouldCancel)
      {
        return result;
      }
      auto writeResult = destination.copyFromBuffer(begin * 3, {output->data(), count * 3});
      result = MergeResults(result, std::move(writeResult));
      if(result.invalid() || shouldCancel)
      {
        return result;
      }
      if(copyNodes)
      {
        auto nodeWriteResult = destinationNodes->copyFromBuffer(begin, {nodeOutput->data(), count});
        result = MergeResults(result, std::move(nodeWriteResult));
        if(result.invalid() || shouldCancel)
        {
          return result;
        }
      }
      begin += count;
      progress.updatePercent(begin);
    }
  } catch(const std::bad_alloc&)
  {
    return MergeResults(result, MakeErrorResult(-57410, "Triple line vertex buffer allocation failed. Free memory or reduce the cache memory budget before retrying."));
  } catch(const std::length_error&)
  {
    return MergeResults(result, MakeErrorResult(-57410, "Triple line vertex buffer size exceeds the container limit. Reduce the mesh size before retrying."));
  }
  return result;
}

// -----------------------------------------------------------------------------
/**
 * @brief Publishes compact edge endpoints and region counts in admitted contiguous blocks.
 * @param destination Receives two compact vertex indices per edge.
 * @param counts Receives one region count per edge.
 * @param edges Supplies sorted source edge keys and saturated region counts.
 * @param indices Supplies ascending compact-to-source vertex indices.
 * @param shouldCancel Signals cancellation between transfers and compacted edges.
 * @param progress Receives completed block progress.
 * @return The first transfer error, admission/allocation error, or accumulated warnings.
 */
Result<> writeTripleLineEdges(AbstractDataStore<IGeometry::MeshIndexType>& destination, Int8AbstractDataStore& counts, nonstd::span<const std::pair<uint64, uint8>> edges,
                              const std::vector<uint64>& indices, const std::atomic_bool& shouldCancel, ThrottledMessageHandler& progress)
{
  if(edges.empty() || shouldCancel)
  {
    return {};
  }
  constexpr usize bytesPerEdge = (2 * sizeof(IGeometry::MeshIndexType)) + sizeof(int8);
  if(edges.size() > std::numeric_limits<usize>::max() / bytesPerEdge)
  {
    return MakeErrorResult(-57411, "Triple line edge transfer sizes exceed addressable memory. Reduce the mesh size before extracting triple lines.");
  }
  const usize requestedBytes = edges.size() * bytesPerEdge;
  auto reservation = CacheMemoryBudgetManager::instance().reserveWorkingMemory(requestedBytes);
  const usize capacity = static_cast<usize>(std::min<uint64>(reservation.sizeBytes() / bytesPerEdge, edges.size()));
  if(capacity == 0)
  {
    return MakeErrorResult(-57409, fmt::format("Triple line edge writing requested {} working bytes and received {}. At least {} bytes are required. Increase the cache memory budget or finish "
                                               "other processing before retrying.",
                                               requestedBytes, reservation.sizeBytes(), bytesPerEdge));
  }
  reservation.shrinkTo(capacity * bytesPerEdge);
  Result<> result;
  try
  {
    auto endpoints = std::make_unique<std::vector<IGeometry::MeshIndexType>>(capacity * 2);
    auto regionCounts = std::make_unique<std::vector<int8>>(capacity);
    for(usize begin = 0; begin < edges.size();)
    {
      const usize count = std::min(capacity, edges.size() - begin);
      for(usize local = 0; local < count; ++local)
      {
        if(shouldCancel)
        {
          return result;
        }
        const auto [key, regions] = edges[begin + local];
        (*endpoints)[local * 2] = static_cast<uint64>(std::ranges::lower_bound(indices, key >> 32) - indices.begin());
        (*endpoints)[(local * 2) + 1] = static_cast<uint64>(std::ranges::lower_bound(indices, key & k_VertexIndexMask) - indices.begin());
        (*regionCounts)[local] = static_cast<int8>(regions);
      }
      if(shouldCancel)
      {
        return result;
      }
      // Copy the accumulator so a throwing merge cannot consume its earlier diagnostics.
      auto writeResult = destination.copyFromBuffer(begin * 2, {endpoints->data(), count * 2});
      result = MergeResults(result, std::move(writeResult));
      if(result.invalid() || shouldCancel)
      {
        return result;
      }
      auto countWriteResult = counts.copyFromBuffer(begin, {regionCounts->data(), count});
      result = MergeResults(result, std::move(countWriteResult));
      if(result.invalid() || shouldCancel)
      {
        return result;
      }
      begin += count;
      progress.updatePercent(begin);
    }
  } catch(const std::bad_alloc&)
  {
    return MergeResults(result, MakeErrorResult(-57412, "Triple line edge buffer allocation failed. Free memory or reduce the cache memory budget before retrying."));
  } catch(const std::length_error&)
  {
    return MergeResults(result, MakeErrorResult(-57412, "Triple line edge buffer size exceeds the container limit. Reduce the mesh size before retrying."));
  }
  return result;
}
} // namespace

namespace nx::core
{
ExtractTripleLines::ExtractTripleLines(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ExtractTripleLinesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ExtractTripleLines::~ExtractTripleLines() noexcept = default;

Result<> ExtractTripleLines::operator()()
{
  if((m_InputValues->SourceNodeTypes == nullptr) != (m_InputValues->DestinationNodeTypes == nullptr))
  {
    return MakeErrorResult(-57401, fmt::format("Triple line Node Types copy requires both source and destination stores, or neither. Source supplied: {}; destination supplied: {}.",
                                               m_InputValues->SourceNodeTypes != nullptr, m_InputValues->DestinationNodeTypes != nullptr));
  }
  if(m_InputValues->VertexBatchSize == 0)
  {
    return MakeErrorResult(-57404, "Triple line vertex batch size must be greater than zero. Set VertexBatchSize to a positive vertex count.");
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto& tripleLineGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->TripleLineGeometryPath);
  const auto& faceLabelsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsPath).getDataStoreRef();
  auto& numFeaturesRef = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NumFeaturesPath).getDataStoreRef();
  const usize numVertices = triangleGeom.getNumberOfVertices();
  if(numVertices > k_MaxVertexCount)
  {
    return MakeErrorResult(-57400, fmt::format("Triple line generation supports meshes with at most {} vertices, but '{}' has {}. The edge lookup key packs two 32-bit vertex indices.",
                                               k_MaxVertexCount, triangleGeom.getName(), numVertices));
  }

  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const usize numTriangles = triangleGeom.getNumberOfFaces();
  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  std::unordered_map<uint64, FeatureSet<4>> edgeMap;
  {
    // Vertex candidates reject ordinary boundary edges before they require hash nodes.
    std::vector<bool> candidateVertices(numVertices, false);
    const usize batchSize = m_InputValues->VertexBatchSize;
    const usize numBatches = numVertices / batchSize + (numVertices % batchSize != 0 ? 1 : 0);
    for(usize begin = 0, batchIndex = 0; begin < numVertices; batchIndex++)
    {
      const usize end = begin + std::min(batchSize, numVertices - begin);
      std::vector<FeatureSet<3>> vertexFeatures(end - begin);
      progressThrottle.reset(numTriangles, fmt::format("Triple Lines: Classifying vertices (batch {}/{})", batchIndex + 1, numBatches));
      for(usize triangleIdx = 0; triangleIdx < numTriangles; triangleIdx++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const std::array<int32, 2> labels = {NormalizeLabel(faceLabelsRef[triangleIdx * 2]), NormalizeLabel(faceLabelsRef[triangleIdx * 2 + 1])};
        for(usize cornerIdx = 0; cornerIdx < 3; cornerIdx++)
        {
          const uint64 vertexIndex = facesRef[triangleIdx * 3 + cornerIdx];
          if(batchIndex == 0 && vertexIndex >= numVertices)
          {
            return MakeErrorResult(
                -57406,
                fmt::format("Triangle {} of '{}' references vertex {}, but the geometry has only {} vertices. Fix the face list (for example, rerun the mesher) before extracting triple lines.",
                            triangleIdx, triangleGeom.getName(), vertexIndex, numVertices));
          }
          if(vertexIndex < begin || vertexIndex >= end)
          {
            continue;
          }
          auto& featureSet = vertexFeatures[vertexIndex - begin];
          for(const int32 label : labels)
          {
            if(m_InputValues->IncludeExteriorLines || label >= 0)
            {
              featureSet.insert(label);
            }
          }
        }
        progressThrottle.updatePercent(triangleIdx + 1);
      }
      for(usize vertexIndex = begin; vertexIndex < end; vertexIndex++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        candidateVertices[vertexIndex] = vertexFeatures[vertexIndex - begin].count() == 3;
      }
      begin = end;
    }

    progressThrottle.reset(numTriangles, "Triple Lines: Classifying candidate edges");
    for(usize triangleIdx = 0; triangleIdx < numTriangles; triangleIdx++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const std::array<int32, 2> labels = {NormalizeLabel(faceLabelsRef[triangleIdx * 2]), NormalizeLabel(faceLabelsRef[triangleIdx * 2 + 1])};
      const std::array<uint64, 3> vertices = {facesRef[triangleIdx * 3], facesRef[triangleIdx * 3 + 1], facesRef[triangleIdx * 3 + 2]};
      for(usize edgeIdx = 0; edgeIdx < 3; edgeIdx++)
      {
        const uint64 vertex0 = vertices[edgeIdx];
        const uint64 vertex1 = vertices[(edgeIdx + 1) % 3];
        // Every triangle containing an edge contains both endpoints. The edge label set is a subset of each endpoint's set.
        // Thus edge count >= 3 implies both endpoint counts >= 3, even though the batched vertex sets saturate at 3.
        if(!candidateVertices[vertex0] || !candidateVertices[vertex1])
        {
          continue;
        }
        auto& featureSet = edgeMap[MakeEdgeKey(vertex0, vertex1)];
        for(const int32 label : labels)
        {
          if(m_InputValues->IncludeExteriorLines || label >= 0)
          {
            featureSet.insert(label);
          }
        }
      }
      progressThrottle.updatePercent(triangleIdx + 1);
    }
  }

  std::vector<std::pair<uint64, uint8>> keptEdges;
  progressThrottle.reset(edgeMap.size(), "Triple Lines: Selecting edges");
  usize processedEdges = 0;
  for(const auto& [edgeKey, featureSet] : edgeMap)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(featureSet.count() >= 3)
    {
      keptEdges.emplace_back(edgeKey, featureSet.count());
    }
    progressThrottle.updatePercent(++processedEdges);
  }
  std::sort(keptEdges.begin(), keptEdges.end());

  std::vector<uint64> compactToOriginal;
  compactToOriginal.reserve(keptEdges.size() * 2);
  for(const auto& [edgeKey, count] : keptEdges)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    compactToOriginal.push_back(edgeKey >> 32);
    compactToOriginal.push_back(edgeKey & k_VertexIndexMask);
  }
  std::sort(compactToOriginal.begin(), compactToOriginal.end());
  compactToOriginal.erase(std::unique(compactToOriginal.begin(), compactToOriginal.end()), compactToOriginal.end());
  if(m_ShouldCancel)
  {
    return {};
  }

  const usize numTripleLineEdges = keptEdges.size();
  const usize numTripleLineVertices = compactToOriginal.size();
  Result<> resizeResult = tripleLineGeom.resizeVertexList(numTripleLineVertices);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = tripleLineGeom.resizeEdgeList(numTripleLineEdges);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = tripleLineGeom.getVertexAttributeMatrix()->resizeTuples({numTripleLineVertices});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = tripleLineGeom.getEdgeAttributeMatrix()->resizeTuples({numTripleLineEdges});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  progressThrottle.reset(numTripleLineVertices, "Triple Lines: Copying vertices");
  const auto& sourceVertsRef = triangleGeom.getVertices()->getDataStoreRef();
  auto& destVertsRef = tripleLineGeom.getVertices()->getDataStoreRef();
  const bool copyVerticesInBlocks = sourceVertsRef.getStoreType() == IDataStore::StoreType::OutOfCore || destVertsRef.getStoreType() == IDataStore::StoreType::OutOfCore ||
                                    (m_InputValues->SourceNodeTypes != nullptr && (m_InputValues->SourceNodeTypes->getStoreType() == IDataStore::StoreType::OutOfCore ||
                                                                                   m_InputValues->DestinationNodeTypes->getStoreType() == IDataStore::StoreType::OutOfCore));
  Result<> publicationResult;
  if(copyVerticesInBlocks)
  {
    publicationResult = copyTripleLineVertices(sourceVertsRef, destVertsRef, {compactToOriginal.data(), compactToOriginal.size()}, m_InputValues->SourceNodeTypes, m_InputValues->DestinationNodeTypes,
                                               m_ShouldCancel, progressThrottle);
    if(publicationResult.invalid() || m_ShouldCancel)
    {
      return publicationResult;
    }
    progressThrottle.flush();
  }
  else
  {
    for(usize vertexIdx = 0; vertexIdx < numTripleLineVertices; vertexIdx++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const uint64 originalVertex = compactToOriginal[vertexIdx];
      for(usize compIdx = 0; compIdx < 3; compIdx++)
      {
        destVertsRef[vertexIdx * 3 + compIdx] = sourceVertsRef[originalVertex * 3 + compIdx];
      }
      if(m_InputValues->SourceNodeTypes != nullptr && m_InputValues->DestinationNodeTypes != nullptr)
      {
        (*m_InputValues->DestinationNodeTypes)[vertexIdx] = (*m_InputValues->SourceNodeTypes)[originalVertex];
      }
      progressThrottle.updatePercent(vertexIdx + 1);
    }
  }

  progressThrottle.reset(numTripleLineEdges, "Triple Lines: Writing edges");
  auto& destEdgesRef = tripleLineGeom.getEdges()->getDataStoreRef();
  if(destEdgesRef.getStoreType() == IDataStore::StoreType::OutOfCore || numFeaturesRef.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    publicationResult =
        MergeResults(std::move(publicationResult), writeTripleLineEdges(destEdgesRef, numFeaturesRef, {keptEdges.data(), keptEdges.size()}, compactToOriginal, m_ShouldCancel, progressThrottle));
    if(publicationResult.invalid() || m_ShouldCancel)
    {
      return publicationResult;
    }
    progressThrottle.flush();
  }
  else
  {
    for(usize edgeIdx = 0; edgeIdx < numTripleLineEdges; edgeIdx++)
    {
      if(m_ShouldCancel)
      {
        return publicationResult;
      }
      const auto [edgeKey, count] = keptEdges[edgeIdx];
      const std::array<uint64, 2> endpoints = {edgeKey >> 32, edgeKey & k_VertexIndexMask};
      for(usize endpointIdx = 0; endpointIdx < endpoints.size(); endpointIdx++)
      {
        destEdgesRef[edgeIdx * 2 + endpointIdx] = static_cast<uint64>(std::lower_bound(compactToOriginal.begin(), compactToOriginal.end(), endpoints[endpointIdx]) - compactToOriginal.begin());
      }
      numFeaturesRef[edgeIdx] = static_cast<int8>(count);
      progressThrottle.updatePercent(edgeIdx + 1);
    }
  }
  if(!m_ShouldCancel && numTripleLineEdges == 0)
  {
    return MakeWarningVoidResult(
        -57405, fmt::format("No edge borders three or more Features (Include Exterior Triple Lines: {}). If this mesh should contain triple lines, check that its triangles share vertices. "
                            "A mesh with duplicated vertices, such as an imported STL, has no shared edges; merge coincident vertices first.",
                            m_InputValues->IncludeExteriorLines));
  }
  return publicationResult;
}
} // namespace nx::core
