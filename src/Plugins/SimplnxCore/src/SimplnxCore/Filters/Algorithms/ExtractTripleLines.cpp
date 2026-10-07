#include "ExtractTripleLines.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
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

  progressThrottle.reset(numTripleLineEdges, "Triple Lines: Writing edges");
  auto& destEdgesRef = tripleLineGeom.getEdges()->getDataStoreRef();
  for(usize edgeIdx = 0; edgeIdx < numTripleLineEdges; edgeIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
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
  if(!m_ShouldCancel && numTripleLineEdges == 0)
  {
    return MakeWarningVoidResult(-57405, "No edge borders three or more Features with the selected exterior option. The Triangle Geometry must share vertices between triangles. "
                                         "A mesh with duplicated vertices, such as an imported STL, does not share edges. Merge coincident vertices before extracting triple lines.");
  }
  return {};
}
} // namespace nx::core
