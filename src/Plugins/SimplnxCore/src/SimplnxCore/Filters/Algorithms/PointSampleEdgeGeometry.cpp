#include "PointSampleEdgeGeometry.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{

template <typename T>
class CopyEdgeDataToVertexData
{
public:
  using StoreType = AbstractDataStore<T>;

  CopyEdgeDataToVertexData(const IDataArray* selectedEdgeArray, IDataArray* createdVertexArray, const UInt64AbstractDataStore& vertexEdgeIds, const std::atomic_bool& shouldCancel)
  : m_SelectedEdgeDataStore(selectedEdgeArray->template getIDataStoreRefAs<StoreType>())
  , m_CreatedVertexDataStore(createdVertexArray->template getIDataStoreRefAs<StoreType>())
  , m_VertexEdgeDataStore(vertexEdgeIds)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    const usize numComps = m_SelectedEdgeDataStore.getNumberOfComponents();

    for(usize vertIdx = range.min(); vertIdx < range.max(); ++vertIdx)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize edgeTupleIdx = m_VertexEdgeDataStore[vertIdx];
      for(usize compIdx = 0; compIdx < numComps; compIdx++)
      {
        m_CreatedVertexDataStore.setComponent(vertIdx, compIdx, m_SelectedEdgeDataStore[edgeTupleIdx * numComps + compIdx]);
      }
    }
  }

private:
  const StoreType& m_SelectedEdgeDataStore;
  StoreType& m_CreatedVertexDataStore;
  const UInt64AbstractDataStore& m_VertexEdgeDataStore;
  const std::atomic_bool& m_ShouldCancel;
};

Result<std::tuple<float32, float32, float32, float32>> computeEdgeVector(const Float32Array& edgeVertices, const UInt64Array& edges, int64 edgeIndex)
{
  const usize endpoint1 = edges[2 * edgeIndex];
  const usize endpoint2 = edges[2 * edgeIndex + 1];
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
  return static_cast<usize>(dist / samplingRes) + 1;
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

Result<> sampleEdge(int64 edgeIndex, const Float32Array& edgeVertices, UInt64Array& edges, float32 samplingRes, Float32Array& vertices, UInt64Array* pointEdgeIdsArrayPtr,
                    Float32Array* cumulativeSampleDistArrayPtr, int64& vertCount)
{
  auto result = computeEdgeVector(edgeVertices, edges, edgeIndex);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  auto [dx, dy, dz, dist] = result.value();
  auto sampleCount = static_cast<int32>(computeNumSamplePoints(dist, samplingRes));
  const float32 step = dist / static_cast<float32>(sampleCount);

  const float32 delX = step * dx / dist;
  const float32 delY = step * dy / dist;
  const float32 delZ = step * dz / dist;
  const float32 halfDelX = delX * 0.5f;
  const float32 halfDelY = delY * 0.5f;
  const float32 halfDelZ = delZ * 0.5f;
  const auto endpoint1 = edges[2 * edgeIndex];
  const float32 startX = edgeVertices[3 * endpoint1] + halfDelX;
  const float32 startY = edgeVertices[3 * endpoint1 + 1] + halfDelY;
  const float32 startZ = edgeVertices[3 * endpoint1 + 2] + halfDelZ;

  for(int32 j = 0; j < sampleCount; ++j)
  {
    vertices[3 * vertCount] = startX + delX * static_cast<float32>(j);
    vertices[3 * vertCount + 1] = startY + delY * static_cast<float32>(j);
    vertices[3 * vertCount + 2] = startZ + delZ * static_cast<float32>(j);

    if(pointEdgeIdsArrayPtr != nullptr)
    {
      pointEdgeIdsArrayPtr->setValue(vertCount, edgeIndex);
    }
    if(cumulativeSampleDistArrayPtr != nullptr)
    {
      cumulativeSampleDistArrayPtr->setValue(vertCount, step * static_cast<float32>(j));
    }
    ++vertCount;
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
PointSampleEdgeGeometry::PointSampleEdgeGeometry(DataStructure& dataStructure, PointSampleEdgeGeometryInputValues* inputValues, const IFilter::MessageHandler& msgHandler,
                                                 const std::atomic_bool& shouldCancel)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
PointSampleEdgeGeometry::~PointSampleEdgeGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> PointSampleEdgeGeometry::operator()()
{
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->ScanVectorGeometryPath);
  auto numEdges = edgeGeom.getNumberOfEdges();

  auto& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->SampledVertexGeometryPath);

  DataPath vertexEdgeIdsDataPath = m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(m_InputValues->EdgeIdsArrayName);
  auto* vertexEdgeIdsDataArrayPtr = m_DataStructure.getDataAs<UInt64Array>(vertexEdgeIdsDataPath);

  auto& vertexEdgeIdsDataStore = vertexEdgeIdsDataArrayPtr->getDataStoreRef();

  Float32Array* cumulativeSampleDistArrayPtr = nullptr;
  if(m_InputValues->CalculateCumulativeSampleDistance)
  {
    cumulativeSampleDistArrayPtr = m_DataStructure.getDataAs<Float32Array>(
        m_InputValues->SampledVertexGeometryPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(m_InputValues->CumulativeSampleDistanceArrayName));
  }

  // --- Step 1: Count how many total sample points to generate ---
  m_MessageHandler.sendInfoMessage("Computing total sampling points...");
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

  m_MessageHandler.sendInfoMessage(fmt::format("Total sampling points: {}", numVertices));

  // Resize the vertex geometry and the vertex attribute matrix
  auto& vertices = vertexGeom.getVerticesRef();
  vertices.resizeTuples({numVertices});
  auto& vertexAttrMatrix = vertexGeom.getVertexAttributeMatrixRef();
  vertexAttrMatrix.resizeTuples({numVertices});

  // --- Step 2: Generate and write each sampled point ---
  m_MessageHandler.sendInfoMessage("Generating sampled points along edge geometry...");
  int64 vertCount = 0;
  for(int64 i = 0; i < numEdges; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    sampleEdge(i, edgeVertices, edges, m_InputValues->ScanVectorSamplingRes, vertices, vertexEdgeIdsDataArrayPtr, cumulativeSampleDistArrayPtr, vertCount);
  }

  usize maxEdgeId = *std::max_element(vertexEdgeIdsDataStore.begin(), vertexEdgeIdsDataStore.end());

  m_MessageHandler.sendInfoMessage("Copying Edge Data to Vertex Geometry...");
  const DataPath vertexAttrMatPath = vertexGeom.getVertexAttributeMatrixDataPath();
  for(const auto& selectedArrayPath : m_InputValues->pSelectedDataArrayPaths)
  {
    DataPath createdArrayPath = vertexAttrMatPath.createChildPath(selectedArrayPath.getTargetName());
    const auto* selectedEdgeArray = m_DataStructure.getDataAs<IDataArray>(selectedArrayPath);
    auto* createdVertexArray = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

    if(maxEdgeId >= selectedEdgeArray->getNumberOfTuples())
    {
      return MakeErrorResult(-45362, fmt::format("Generated Vertex Array '{}' has a maximum EdgeId value of {}. This is greater than the number of tuples ({}) in data array '{}'",
                                                 vertexEdgeIdsDataPath.toString(), maxEdgeId, selectedEdgeArray->getNumberOfTuples(), selectedArrayPath.toString()));
    }

    m_MessageHandler.sendInfoMessage(fmt::format("Copying data into vertex array '{}'...", createdArrayPath.toString()));
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, createdVertexArray->getNumberOfTuples());
    ExecuteParallelFunction<::CopyEdgeDataToVertexData>(selectedEdgeArray->getDataType(), dataAlg, selectedEdgeArray, createdVertexArray, vertexEdgeIdsDataStore, m_ShouldCancel);
  }

  return {};
}
