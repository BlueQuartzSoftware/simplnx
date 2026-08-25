#include "TripleLineUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <unordered_map>
#include <vector>

using namespace nx::core;

namespace
{
// NumFeatures saturates at 4, matching the NodeTypes convention which also caps at 4.
constexpr uint8 k_MaxFeaturesPerEdge = 4;

// The edge key packs two 32-bit vertex indices into one 64-bit value, so meshes are
// limited to 2^32 vertices. GenerateTripleLines checks this up front.
constexpr uint64 k_MaxVertexCount = 0xFFFFFFFFULL;

// Recovers the low vertex index when unpacking an edge key.
constexpr uint64 k_VertexIndexMask = 0xFFFFFFFFULL;

/**
 * @brief The unique Feature Ids bordering one mesh edge. A flat array with linear-scan
 * insert rather than a std::set: at most 4 entries are ever kept, and a tree node per
 * mesh edge would dominate memory on a multi-million triangle mesh.
 */
struct EdgeFeatureSet
{
  std::array<int32, k_MaxFeaturesPerEdge> Features{};
  uint8 Count = 0;

  void insert(int32 featureId)
  {
    for(uint8 i = 0; i < Count; i++)
    {
      if(Features[i] == featureId)
      {
        return;
      }
    }
    // Saturate rather than grow. An edge bordering more than 4 regions is not
    // geometrically meaningful here, and Count stays a faithful "3 or 4".
    if(Count < k_MaxFeaturesPerEdge)
    {
      Features[Count] = featureId;
      Count++;
    }
  }
};

/**
 * @brief Packs a vertex pair into an order-independent key.
 */
inline uint64 MakeEdgeKey(uint64 v0, uint64 v1)
{
  return (v0 < v1) ? ((v0 << 32) | v1) : ((v1 << 32) | v0);
}
} // namespace

namespace nx::core::MeshingUtilities
{
Result<> GenerateTripleLines(const TriangleGeom& triangleGeom, const Int32AbstractDataStore& faceLabelsRef, const Int8AbstractDataStore& sourceNodeTypesRef, EdgeGeom& tripleLineGeom,
                             Int8AbstractDataStore& numFeaturesRef, Int8AbstractDataStore& tripleLineNodeTypesRef, const TripleLineOptions& options, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
{
  const usize numVertices = triangleGeom.getNumberOfVertices();
  if(numVertices > k_MaxVertexCount)
  {
    return MakeErrorResult(-57100, fmt::format("Triple line generation supports meshes with at most {} vertices, but '{}' has {}. The edge lookup key packs two 32-bit vertex indices.",
                                               k_MaxVertexCount, triangleGeom.getName(), numVertices));
  }

  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const usize numTriangles = triangleGeom.getNumberOfFaces();

  messageHandler(IFilter::Message::Type::Info, "Triple Lines: Building edge adjacency...");

  std::unordered_map<uint64, EdgeFeatureSet> edgeMap;
  edgeMap.reserve(numTriangles * 3 / 2);

  for(usize triIndex = 0; triIndex < numTriangles; triIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const int32 labelA = faceLabelsRef[triIndex * 2 + 0];
    const int32 labelB = faceLabelsRef[triIndex * 2 + 1];

    const uint64 v0 = facesRef[triIndex * 3 + 0];
    const uint64 v1 = facesRef[triIndex * 3 + 1];
    const uint64 v2 = facesRef[triIndex * 3 + 2];

    const std::array<uint64, 3> edgeKeys = {MakeEdgeKey(v0, v1), MakeEdgeKey(v1, v2), MakeEdgeKey(v2, v0)};
    for(const uint64 edgeKey : edgeKeys)
    {
      EdgeFeatureSet& featureSet = edgeMap[edgeKey];
      // When exterior lines are excluded, -1 is simply never counted, so an edge whose
      // only third "region" is the outside of the volume falls back to 2 unique ids.
      if(options.IncludeExteriorLines || labelA >= 0)
      {
        featureSet.insert(labelA);
      }
      if(options.IncludeExteriorLines || labelB >= 0)
      {
        featureSet.insert(labelB);
      }
    }
  }

  messageHandler(IFilter::Message::Type::Info, "Triple Lines: Selecting edges...");

  std::vector<uint64> keptEdges; // pairs of COMPACTED vertex indices
  std::vector<int8> keptFeatureCounts;
  std::unordered_map<uint64, uint64> vertexRemap; // original vertex index -> compact index
  std::vector<uint64> compactToOriginal;

  for(const auto& [edgeKey, featureSet] : edgeMap)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(featureSet.Count < 3)
    {
      continue;
    }

    const uint64 vA = edgeKey >> 32;
    const uint64 vB = edgeKey & k_VertexIndexMask;

    for(const uint64 originalVertex : {vA, vB})
    {
      if(vertexRemap.find(originalVertex) == vertexRemap.end())
      {
        vertexRemap[originalVertex] = compactToOriginal.size();
        compactToOriginal.push_back(originalVertex);
      }
    }

    keptEdges.push_back(vertexRemap[vA]);
    keptEdges.push_back(vertexRemap[vB]);
    keptFeatureCounts.push_back(static_cast<int8>(featureSet.Count));
  }

  const usize numTripleLineEdges = keptFeatureCounts.size();
  const usize numTripleLineVertices = compactToOriginal.size();

  messageHandler(IFilter::Message::Type::Info, fmt::format("Triple Lines: Writing {} edges and {} vertices...", numTripleLineEdges, numTripleLineVertices));

  tripleLineGeom.resizeVertexList(numTripleLineVertices);
  tripleLineGeom.resizeEdgeList(numTripleLineEdges);
  tripleLineGeom.getVertexAttributeMatrix()->resizeTuples({numTripleLineVertices});
  tripleLineGeom.getEdgeAttributeMatrix()->resizeTuples({numTripleLineEdges});
  numFeaturesRef.resizeTuples({numTripleLineEdges});
  tripleLineNodeTypesRef.resizeTuples({numTripleLineVertices});

  const auto& sourceVertsRef = triangleGeom.getVertices()->getDataStoreRef();
  auto& destVertsRef = tripleLineGeom.getVertices()->getDataStoreRef();
  for(usize i = 0; i < numTripleLineVertices; i++)
  {
    const uint64 originalVertex = compactToOriginal[i];
    destVertsRef[i * 3 + 0] = sourceVertsRef[originalVertex * 3 + 0];
    destVertsRef[i * 3 + 1] = sourceVertsRef[originalVertex * 3 + 1];
    destVertsRef[i * 3 + 2] = sourceVertsRef[originalVertex * 3 + 2];
    // Carried across unchanged. NodeTypes never influences which edges were selected above.
    tripleLineNodeTypesRef[i] = sourceNodeTypesRef[originalVertex];
  }

  auto& destEdgesRef = tripleLineGeom.getEdges()->getDataStoreRef();
  for(usize i = 0; i < numTripleLineEdges; i++)
  {
    destEdgesRef[i * 2 + 0] = keptEdges[i * 2 + 0];
    destEdgesRef[i * 2 + 1] = keptEdges[i * 2 + 1];
    numFeaturesRef[i] = keptFeatureCounts[i];
  }

  return {};
}
} // namespace nx::core::MeshingUtilities
