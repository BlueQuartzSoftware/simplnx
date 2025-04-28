#include "SampleScanVectors.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

using namespace nx::core;

namespace
{
Result<std::tuple<float32, float32, float32, float32>> computeEdgeVector(const Float32Array& edgeVertices, const UInt64Array& edges, int64 edgeIndex)
{
  usize endpoint1 = edges[2 * edgeIndex];
  usize endpoint2 = edges[2 * edgeIndex + 1];
  float32 dx = edgeVertices[3 * endpoint2] - edgeVertices[3 * endpoint1];
  float32 dy = edgeVertices[3 * endpoint2 + 1] - edgeVertices[3 * endpoint1 + 1];
  float32 dz = edgeVertices[3 * endpoint2 + 2] - edgeVertices[3 * endpoint1 + 2];
  float32 dist = dx * dx + dy * dy + dz * dz;
  if(dist == 0.0f)
  {
    return MakeErrorResult<std::tuple<float32, float32, float32, float32>>(-14, "Unable to compute edge vector: euclidean distance is 0.");
  }
  dist = std::sqrt(dist);
  return {std::make_tuple(dx, dy, dz, dist)};
}

usize computeNumSamplePoints(float32 dist, float32 samplingRes)
{
  return usize(dist / samplingRes) + 1;
}

Result<usize> countSamplesForEdge(const Float32Array& edgeVertices, const UInt64Array& edges, int64 edgeIndex, float32 samplingRes)
{
  auto result = computeEdgeVector(edgeVertices, edges, edgeIndex);
  if(result.invalid())
  {
    return ConvertResultTo<usize>(std::move(ConvertResult(std::move(result))), {});
  }
  auto [dx, dy, dz, dist] = result.value();
  return {computeNumSamplePoints(dist, samplingRes)};
}

Result<> sampleEdge(int64 edgeIndex, const Float32Array& edgeVertices, UInt64Array& edges, const Float64Array& timeArray, const Float32Array& powerArray, const Int32Array& sliceIdsArray,
                    float32 samplingRes, Float32Array& vertices, Float64Array& pointTimeArray, Float32Array& pointPowerArray, Int32Array& pointSliceIdsArray, Float32Array& cumulativeSampleDistArray,
                    int64& vertCount)
{
  auto v1 = edges[2 * edgeIndex];
  auto v2 = edges[2 * edgeIndex + 1];
  double startTime = timeArray[v1];
  double endTime = timeArray[v2];
  double totalTime = endTime - startTime;

  auto result = computeEdgeVector(edgeVertices, edges, edgeIndex);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  auto [dx, dy, dz, dist] = result.value();
  auto sampleCount = static_cast<int32>(computeNumSamplePoints(dist, samplingRes));
  float32 step = dist / static_cast<float32>(sampleCount);
  double timeDelta = totalTime / static_cast<float64>(sampleCount);

  float delX = step * dx / dist;
  float delY = step * dy / dist;
  float delZ = step * dz / dist;
  float halfDelX = delX * 0.5f;
  float halfDelY = delY * 0.5f;
  float halfDelZ = delZ * 0.5f;
  float startX = edgeVertices[3 * v1] + halfDelX;
  float startY = edgeVertices[3 * v1 + 1] + halfDelY;
  float startZ = edgeVertices[3 * v1 + 2] + halfDelZ;

  for(int32 j = 0; j < sampleCount; ++j)
  {
    vertices[3 * vertCount] = startX + delX * static_cast<float32>(j);
    vertices[3 * vertCount + 1] = startY + delY * static_cast<float32>(j);
    vertices[3 * vertCount + 2] = startZ + delZ * static_cast<float32>(j);
    pointTimeArray[vertCount] = static_cast<float64>(startTime + timeDelta * j + timeDelta * 0.5);
    pointPowerArray[vertCount] = powerArray[edgeIndex];
    pointSliceIdsArray[vertCount] = sliceIdsArray[edgeIndex];
    cumulativeSampleDistArray[vertCount] = step * static_cast<float32>(j);
    ++vertCount;
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
SampleScanVectors::SampleScanVectors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, SampleScanVectorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
SampleScanVectors::~SampleScanVectors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> SampleScanVectors::operator()()
{
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->ScanVectorGeometryPath);
  auto& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->SampledVertexGeometryPath);
  auto& timeArray = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->TimeArrayPath);
  auto& powerArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->PowerArrayPath);
  auto& sliceIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SliceIdArrayPath);

  auto& pointTimeArray =
      m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(timeArray.getName()));
  auto& pointPowerArray =
      m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(powerArray.getName()));
  auto& pointSliceIdsArray =
      m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(sliceIdsArray.getName()));
  auto& cumulativeSampleDistArray = m_DataStructure.getDataRefAs<Float32Array>(
      m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(m_InputValues->CumulativeSampleDistanceArrayName));

  // --- Step 1: Count how many total sample points to generate ---
  m_MessageHandler(IFilter::Message::Type::Info, "Counting total sample points for all scan vectors...");
  usize numEdges = edgeGeom.getNumberOfEdges();
  usize numVertices = 0;

  auto& edgeVertices = edgeGeom.getVerticesRef();
  auto& edges = edgeGeom.getEdgesRef();

  for(int64 i = 0; i < numEdges; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto result = countSamplesForEdge(edgeVertices, edges, i, m_InputValues->ScanVectorSamplingRes);
    if(result.invalid())
    {
      return ConvertResult(std::move(result));
    }
    numVertices += result.value();
  }

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Finished counting.  Will generate {} sampled points.", numVertices));

  // Resize the vertex geometry to hold all sample points
  auto& vertexAttrMatrix = vertexGeom.getVertexAttributeMatrixRef();
  vertexAttrMatrix.resizeTuples({numVertices});
  auto& vertices = vertexGeom.getVerticesRef();
  vertices.resizeTuples({numVertices});

  // --- Step 2: Generate and write each sampled point ---
  m_MessageHandler(IFilter::Message::Type::Info, "Generating sampled points along scan vectors...");
  int64 vertCount = 0;
  for(int64 i = 0; i < numEdges; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    sampleEdge(i, edgeVertices, edges, timeArray, powerArray, sliceIdsArray, m_InputValues->ScanVectorSamplingRes, vertices, pointTimeArray, pointPowerArray, pointSliceIdsArray,
               cumulativeSampleDistArray, vertCount);
  }

  return {};
}
