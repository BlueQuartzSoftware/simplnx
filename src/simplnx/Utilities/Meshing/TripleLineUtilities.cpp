#include "TripleLineUtilities.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
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
constexpr uint8 k_MaxFeatures = 4;
// Each half of an edge key holds one 32-bit source vertex index.
constexpr uint64 k_MaxVertexCount = std::numeric_limits<uint32>::max();
constexpr uint64 k_VertexIndexMask = k_MaxVertexCount;

/**
 * @brief Stores at most four unique Feature Ids without per-Feature allocations.
 */
struct FeatureSet
{
  std::array<int32, k_MaxFeatures> Features{};
  uint8 Count = 0;

  void insert(int32 featureId)
  {
    for(uint8 featureIdx = 0; featureIdx < Count; featureIdx++)
    {
      if(Features[featureIdx] == featureId)
      {
        return;
      }
    }
    // Four or more Features have the same output classification.
    if(Count < k_MaxFeatures)
    {
      Features[Count] = featureId;
      Count++;
    }
  }
};

constexpr uint64 MakeEdgeKey(uint64 vertex0, uint64 vertex1)
{
  return (vertex0 < vertex1) ? ((vertex0 << 32) | vertex1) : ((vertex1 << 32) | vertex0);
}
} // namespace

namespace nx::core::MeshingUtilities
{
Result<> GenerateTripleLines(const TripleLineInputs& inputs, const TripleLineOutputs& outputs, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if((inputs.NodeTypes == nullptr) != (outputs.NodeTypes == nullptr))
  {
    return MakeErrorResult(-57401, fmt::format("Triple line Node Types copy requires both source and destination stores, or neither. Source supplied: {}; destination supplied: {}.",
                                               inputs.NodeTypes != nullptr, outputs.NodeTypes != nullptr));
  }
  if(shouldCancel)
  {
    return {};
  }
  const auto& triangleGeom = inputs.TriangleGeometry;
  auto& tripleLineGeom = outputs.TripleLineGeometry;
  const usize numVertices = triangleGeom.getNumberOfVertices();
  if(numVertices > k_MaxVertexCount)
  {
    return MakeErrorResult(-57400, fmt::format("Triple line generation supports meshes with at most {} vertices, but '{}' has {}. The edge lookup key packs two 32-bit vertex indices.",
                                               k_MaxVertexCount, triangleGeom.getName(), numVertices));
  }

  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const usize numTriangles = triangleGeom.getNumberOfFaces();
  ThrottledMessageHandler progressThrottle(messageHandler);
  std::unordered_map<uint64, FeatureSet> edgeMap;
  {
    // Vertex sets reject ordinary boundary edges before they require hash nodes.
    std::vector<FeatureSet> vertexFeatures(numVertices);
    progressThrottle.reset(numTriangles, "Triple Lines: Classifying vertices");
    for(usize triangleIdx = 0; triangleIdx < numTriangles; triangleIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }
      const std::array<int32, 2> labels = {inputs.FaceLabels[triangleIdx * 2], inputs.FaceLabels[triangleIdx * 2 + 1]};
      for(usize cornerIdx = 0; cornerIdx < 3; cornerIdx++)
      {
        auto& featureSet = vertexFeatures[facesRef[triangleIdx * 3 + cornerIdx]];
        for(const int32 label : labels)
        {
          if(inputs.IncludeExteriorLines || label >= 0)
          {
            featureSet.insert(label);
          }
        }
      }
      progressThrottle.updatePercent(triangleIdx + 1);
    }

    progressThrottle.reset(numTriangles, "Triple Lines: Classifying candidate edges");
    for(usize triangleIdx = 0; triangleIdx < numTriangles; triangleIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }
      const std::array<int32, 2> labels = {inputs.FaceLabels[triangleIdx * 2], inputs.FaceLabels[triangleIdx * 2 + 1]};
      const std::array<uint64, 3> vertices = {facesRef[triangleIdx * 3], facesRef[triangleIdx * 3 + 1], facesRef[triangleIdx * 3 + 2]};
      for(usize edgeIdx = 0; edgeIdx < 3; edgeIdx++)
      {
        const uint64 vertex0 = vertices[edgeIdx];
        const uint64 vertex1 = vertices[(edgeIdx + 1) % 3];
        // Every triangle containing an edge contains both endpoints. The edge label set is a subset of each endpoint's set.
        // Thus edge count >= 3 implies both endpoint counts >= 3, including when the sets saturate at 4.
        if(vertexFeatures[vertex0].Count < 3 || vertexFeatures[vertex1].Count < 3)
        {
          continue;
        }
        auto& featureSet = edgeMap[MakeEdgeKey(vertex0, vertex1)];
        for(const int32 label : labels)
        {
          if(inputs.IncludeExteriorLines || label >= 0)
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
    if(shouldCancel)
    {
      return {};
    }
    if(featureSet.Count >= 3)
    {
      keptEdges.emplace_back(edgeKey, featureSet.Count);
    }
    progressThrottle.updatePercent(++processedEdges);
  }
  std::sort(keptEdges.begin(), keptEdges.end());

  std::vector<uint64> compactToOriginal;
  compactToOriginal.reserve(keptEdges.size() * 2);
  for(const auto& [edgeKey, count] : keptEdges)
  {
    if(shouldCancel)
    {
      return {};
    }
    compactToOriginal.push_back(edgeKey >> 32);
    compactToOriginal.push_back(edgeKey & k_VertexIndexMask);
  }
  std::sort(compactToOriginal.begin(), compactToOriginal.end());
  compactToOriginal.erase(std::unique(compactToOriginal.begin(), compactToOriginal.end()), compactToOriginal.end());
  if(shouldCancel)
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
    if(shouldCancel)
    {
      return {};
    }
    const uint64 originalVertex = compactToOriginal[vertexIdx];
    for(usize compIdx = 0; compIdx < 3; compIdx++)
    {
      destVertsRef[vertexIdx * 3 + compIdx] = sourceVertsRef[originalVertex * 3 + compIdx];
    }
    if(inputs.NodeTypes != nullptr && outputs.NodeTypes != nullptr)
    {
      (*outputs.NodeTypes)[vertexIdx] = (*inputs.NodeTypes)[originalVertex];
    }
    progressThrottle.updatePercent(vertexIdx + 1);
  }

  progressThrottle.reset(numTripleLineEdges, "Triple Lines: Writing edges");
  auto& destEdgesRef = tripleLineGeom.getEdges()->getDataStoreRef();
  for(usize edgeIdx = 0; edgeIdx < numTripleLineEdges; edgeIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }
    const auto [edgeKey, count] = keptEdges[edgeIdx];
    const std::array<uint64, 2> endpoints = {edgeKey >> 32, edgeKey & k_VertexIndexMask};
    for(usize endpointIdx = 0; endpointIdx < endpoints.size(); endpointIdx++)
    {
      destEdgesRef[edgeIdx * 2 + endpointIdx] = static_cast<uint64>(std::lower_bound(compactToOriginal.begin(), compactToOriginal.end(), endpoints[endpointIdx]) - compactToOriginal.begin());
    }
    outputs.NumFeatures[edgeIdx] = static_cast<int8>(count);
    progressThrottle.updatePercent(edgeIdx + 1);
  }
  return {};
}
} // namespace nx::core::MeshingUtilities
