

#include "LaplacianSmoothing.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/ProgressEstimator.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
using SmoothingIndexType = IGeometry::MeshIndexType;

/** @brief Owns only the newly added mesh copies and their admitted working bytes. */
struct SmoothingWorkspace
{
  CacheMemoryBudgetManager::WorkingMemoryReservation Reservation;
  std::vector<float32> Vertices;
  std::vector<SmoothingIndexType> Edges;
  std::vector<usize> Order;
  std::vector<float32> Gathered;
  std::vector<float32> Page;
  std::unique_ptr<ITemporaryRecordStore> Temporary;
  usize EdgeCapacity = 0;
  usize PublicationCapacity = 0;
  bool CacheVertices = false;
  bool CacheEdges = false;
};

/**
 * @brief Releases compute buffers before their memory reservation.
 * @param work Owns compute buffers, the reservation, and temporary coordinate records.
 * @note Retains temporary coordinate records for publication.
 */
void releaseSmoothingCompute(SmoothingWorkspace& work)
{
  std::vector<float32>().swap(work.Vertices);
  std::vector<SmoothingIndexType>().swap(work.Edges);
  std::vector<usize>().swap(work.Order);
  std::vector<float32>().swap(work.Gathered);
  std::vector<float32>().swap(work.Page);
  work.Reservation = {};
}

float32 smoothingLambda(int8 nodeType, const LaplacianSmoothingInputValues& input)
{
  switch(nodeType)
  {
  case NodeType::Default:
    return input.pLambda;
  case NodeType::TriplePoint:
    return input.pTripleLineLambda;
  case NodeType::QuadPoint:
    return input.pQuadPointLambda;
  case NodeType::SurfaceDefault:
    return input.pSurfacePointLambda;
  case NodeType::SurfaceTriplePoint:
    return input.pSurfaceTripleLineLambda;
  case NodeType::SurfaceQuadPoint:
    return input.pSurfaceQuadPointLambda;
  default:
    return 0.0F;
  }
}

Result<> readSmoothingLambdas(const Int8AbstractDataStore& nodes, std::vector<float32>& lambdas, const LaplacianSmoothingInputValues& input)
{
  const usize count = nodes.getNumberOfTuples();
  lambdas.assign(count, 0.0F);
  if(count == 0)
  {
    return {};
  }
  auto& budget = CacheMemoryBudgetManager::instance();
  auto reservation = budget.reserveWorkingMemory(std::min<uint64>(count, budget.maximumWorkingMemoryBytes()));
  const usize capacity = static_cast<usize>(std::min<uint64>(count, reservation.sizeBytes() / sizeof(int8)));
  if(capacity == 0)
  {
    return MakeErrorResult(-564, fmt::format("Smoothing node types at '{}' requested {} working bytes and received {}. At least {} bytes are required.", input.pSurfaceMeshNodeTypeArrayPath.toString(),
                                             count, reservation.sizeBytes(), sizeof(int8)));
  }
  std::vector<int8> page(capacity);
  Result<> result;
  for(usize first = 0; first < count;)
  {
    const usize size = std::min(capacity, count - first);
    result = MergeResults(std::move(result), nodes.copyIntoBuffer(first, {page.data(), size}));
    if(result.invalid())
    {
      return MergeResults(std::move(result),
                          MakeErrorResult(-560, fmt::format("Error reading node types at path '{}' (first vertex {}, count {}).", input.pSurfaceMeshNodeTypeArrayPath.toString(), first, size)));
    }
    for(usize index = 0; index < size; ++index)
    {
      lambdas[first + index] = smoothingLambda(page[index], input);
    }
    first += size;
  }
  return result;
}

Result<> readTemporaryCoordinates(const ITemporaryRecordStore& temporary, usize first, nonstd::span<float32> page)
{
  const std::atomic_bool active{false};
  const usize count = page.size() / 3;
  const auto bytes = std::as_writable_bytes(std::span<float32>(page.data(), page.size()));
  auto read = temporary.read(first, count, {bytes.data(), bytes.size()}, active);
  if(read.invalid())
  {
    return ConvertResult(std::move(read));
  }
  if(read.value() != count)
  {
    return MergeResults(ConvertResult(std::move(read)),
                        MakeErrorResult(-565, fmt::format("Smoothing temporary coordinates returned a short read at vertex {}: expected {} XYZ records.", first, count)));
  }
  return ConvertResult(std::move(read));
}

Result<> writeTemporaryCoordinates(ITemporaryRecordStore& temporary, usize first, nonstd::span<const float32> page)
{
  const std::atomic_bool active{false};
  const auto bytes = std::as_bytes(std::span<const float32>(page.data(), page.size()));
  return temporary.write(first, page.size() / 3, {bytes.data(), bytes.size()}, active);
}

Result<> prepareSmoothingWorkspace(SmoothingWorkspace& work, const Float32AbstractDataStore& vertices, const AbstractDataStore<SmoothingIndexType>& edges)
{
  constexpr usize maximum = std::numeric_limits<usize>::max();
  constexpr usize vertexUnit = 3 * sizeof(float32);
  constexpr usize edgeUnit = 2 * sizeof(SmoothingIndexType);
  constexpr usize gatherUnit = edgeUnit + (2 * sizeof(usize)) + (12 * sizeof(float32));
  const usize vertexCount = vertices.getNumberOfTuples();
  const usize edgeCount = edges.getNumberOfTuples();
  if(vertexCount > maximum / vertexUnit || edgeCount > maximum / edgeUnit || vertexCount * vertexUnit > maximum - edgeCount * edgeUnit)
  {
    return MakeErrorResult(-566, "Smoothing coordinates and edges exceed the addressable buffer size. Reduce the mesh size.");
  }
  const usize vertexBytes = vertexCount * vertexUnit;
  const usize edgeBytes = edgeCount * edgeUnit;
  const usize requested = vertexBytes + edgeBytes;
  auto& budget = CacheMemoryBudgetManager::instance();
  work.Reservation = budget.reserveWorkingMemory(std::min<uint64>(requested, budget.maximumWorkingMemoryBytes()));
  const usize granted = static_cast<usize>(std::min<uint64>(work.Reservation.sizeBytes(), maximum));
  const usize gatherEdges = std::min(std::max(usize{1}, edgeCount), granted / gatherUnit);
  work.CacheEdges = vertexBytes <= granted && edgeBytes <= granted - vertexBytes;
  // A coordinate-only shortcut must retain the useful edge capacity of the fallback.
  work.CacheVertices = work.CacheEdges || (vertexBytes <= granted && std::max(usize{1}, gatherEdges) <= (granted - vertexBytes) / edgeUnit);
  work.EdgeCapacity = gatherEdges;
  if(work.CacheEdges)
  {
    work.EdgeCapacity = std::max(usize{1}, edgeCount);
  }
  else if(work.CacheVertices)
  {
    work.EdgeCapacity = std::min(std::max(usize{1}, edgeCount), (granted - vertexBytes) / edgeUnit);
  }
  if(work.EdgeCapacity == 0)
  {
    return MakeErrorResult(-567, fmt::format("Smoothing requested {} working bytes and received {}. At least {} bytes are needed for a gathered edge, or {} bytes for cached coordinates and one edge.",
                                             requested, granted, gatherUnit, vertexBytes <= maximum - edgeUnit ? vertexBytes + edgeUnit : maximum));
  }
  work.PublicationCapacity = std::min(vertexCount, std::max(usize{1}, granted / vertexUnit));
  work.Vertices.resize(work.CacheVertices ? vertexCount * 3 : 0);
  work.Edges.resize((work.CacheEdges ? edgeCount : work.EdgeCapacity) * 2);
  Result<> result;
  if(work.CacheVertices)
  {
    result = MergeResults(std::move(result), vertices.copyIntoBuffer(0, {work.Vertices.data(), work.Vertices.size()}));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-561, "Error reading smoothing vertex coordinates."));
    }
  }
  else
  {
    const usize pageVertices = std::min(vertexCount, work.EdgeCapacity * 2);
    work.Order.resize(work.EdgeCapacity * 2);
    work.Gathered.resize(work.EdgeCapacity * 6);
    work.Page.resize(pageVertices * 3);
    work.PublicationCapacity = pageVertices;
    TemporaryRecordStoreConfig config;
    config.recordSize = vertexUnit;
    config.initialRecordCount = vertexCount;
    config.maxRecordsPerBatch = pageVertices;
    config.readOnly = false;
    auto created = DataStoreUtilities::CreateTemporaryRecordStore(config);
    if(created.invalid())
    {
      return MergeResults(std::move(result), ConvertResult(std::move(created)));
    }
    if(created.value() == nullptr)
    {
      return MergeResults(MergeResults(std::move(result), ConvertResult(std::move(created))), MakeErrorResult(-568, "Smoothing temporary-record provider returned a null coordinate store."));
    }
    work.Temporary = std::move(created.value());
    result = MergeResults(std::move(result), ConvertResult(std::move(created)));
    for(usize first = 0; first < vertexCount;)
    {
      const usize count = std::min(pageVertices, vertexCount - first);
      result = MergeResults(std::move(result), vertices.copyIntoBuffer(first * 3, {work.Page.data(), count * 3}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-561, fmt::format("Error reading smoothing coordinates at vertex {} ({} vertices).", first, count)));
      }
      result = MergeResults(std::move(result), writeTemporaryCoordinates(*work.Temporary, first, {work.Page.data(), count * 3}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-569, fmt::format("Error initializing temporary smoothing coordinates at vertex {} ({} vertices).", first, count)));
      }
      first += count;
    }
  }
  if(work.CacheEdges)
  {
    result = MergeResults(std::move(result), edges.copyIntoBuffer(0, {work.Edges.data(), work.Edges.size()}));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-562, "Error reading smoothing edges."));
    }
  }
  return result;
}

Result<> accumulateSmoothingDeltas(SmoothingWorkspace& work, const AbstractDataStore<SmoothingIndexType>& edges, usize vertexCount, std::vector<double>& deltas, std::vector<int32>& connections)
{
  const usize edgeCount = edges.getNumberOfTuples();
  Result<> result;
  for(usize first = 0; first < edgeCount;)
  {
    const usize count = std::min(work.EdgeCapacity, edgeCount - first);
    if(!work.CacheEdges)
    {
      result = MergeResults(std::move(result), edges.copyIntoBuffer(first * 2, {work.Edges.data(), count * 2}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-562, fmt::format("Error reading smoothing edges at edge {} ({} edges).", first, count)));
      }
    }
    const auto block = nonstd::span<const SmoothingIndexType>(work.Edges.data(), work.Edges.size()).subspan(work.CacheEdges ? first * 2 : 0, count * 2);
    for(usize endpoint = 0; endpoint < count * 2; ++endpoint)
    {
      if(block[endpoint] >= vertexCount)
      {
        return MergeResults(std::move(result),
                            MakeErrorResult(-570, fmt::format("Smoothing edge {} references vertex {}, but the vertex count is {}.", first + (endpoint / 2), block[endpoint], vertexCount)));
      }
    }
    if(!work.CacheVertices)
    {
      result = MergeResults(std::move(result), MeshingUtilities::detail::GatherVertexCoordinates(*work.Temporary, block, {work.Order.data(), count * 2}, {work.Gathered.data(), count * 6},
                                                                                                 {work.Page.data(), work.Page.size()}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-571, fmt::format("Error gathering smoothing coordinates for edge {} ({} edges).", first, count)));
      }
    }
    for(usize edge = 0; edge < count; ++edge)
    {
      const usize in1 = static_cast<usize>(block[edge * 2]);
      const usize in2 = static_cast<usize>(block[(edge * 2) + 1]);
      const nonstd::span<const float32> coordinates =
          work.CacheVertices ? nonstd::span<const float32>(work.Vertices.data(), work.Vertices.size()) : nonstd::span<const float32>(work.Gathered.data(), work.Gathered.size());
      const usize firstOffset = work.CacheVertices ? in1 * 3 : edge * 6;
      const usize secondOffset = work.CacheVertices ? in2 * 3 : (edge * 6) + 3;
      const auto firstPoint = coordinates.subspan(firstOffset, 3);
      const auto secondPoint = coordinates.subspan(secondOffset, 3);
      for(usize component = 0; component < 3; ++component)
      {
        const double delta = static_cast<double>(secondPoint[component] - firstPoint[component]);
        deltas[(in1 * 3) + component] += delta;
        deltas[(in2 * 3) + component] += -1.0 * delta;
      }
      connections[in1] += 1;
      connections[in2] += 1;
    }
    first += count;
  }
  return result;
}

Result<> updateSmoothingCoordinates(SmoothingWorkspace& work, const std::vector<float32>& lambdas, std::vector<double>& deltas, std::vector<int32>& connections, bool negativePass, float32 muFactor)
{
  const usize vertexCount = lambdas.size();
  const usize capacity = work.CacheVertices ? std::max(usize{1}, vertexCount) : work.Page.size() / 3;
  Result<> result;
  for(usize first = 0; first < vertexCount;)
  {
    const usize count = std::min(capacity, vertexCount - first);
    const auto coordinates = work.CacheVertices ? nonstd::span<float32>(work.Vertices.data(), work.Vertices.size()).subspan(first * 3, count * 3) :
                                                  nonstd::span<float32>(work.Page.data(), work.Page.size()).first(count * 3);
    if(!work.CacheVertices)
    {
      result = MergeResults(std::move(result), readTemporaryCoordinates(*work.Temporary, first, coordinates));
      if(result.invalid())
      {
        return result;
      }
    }
    for(usize local = 0; local < count; ++local)
    {
      const usize vertex = first + local;
      for(usize component = 0; component < 3; ++component)
      {
        const usize flat = (vertex * 3) + component;
        const double delta = deltas[flat] / connections[vertex];
        const float32 lambda = negativePass ? lambdas[vertex] * muFactor : lambdas[vertex];
        coordinates[(local * 3) + component] += static_cast<float32>(lambda * delta);
        deltas[flat] = 0.0;
      }
      connections[vertex] = 0;
    }
    if(!work.CacheVertices)
    {
      result = MergeResults(std::move(result), writeTemporaryCoordinates(*work.Temporary, first, {coordinates.data(), coordinates.size()}));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-572, fmt::format("Error updating temporary smoothing coordinates at vertex {} ({} vertices).", first, count)));
      }
    }
    first += count;
  }
  return result;
}

Result<> readPublicationCoordinates(ITemporaryRecordStore* temporaryPtr, std::span<const std::byte> staged, usize first, nonstd::span<float32> output)
{
  if(temporaryPtr != nullptr)
  {
    return readTemporaryCoordinates(*temporaryPtr, first, output);
  }
  const usize offset = first * 3 * sizeof(float32);
  const usize bytes = output.size_bytes();
  if(offset > staged.size() || bytes > staged.size() - offset)
  {
    return MakeErrorResult(-573, "Smoothing publication exceeds the completed coordinate byte range.");
  }
  std::memcpy(output.data(), staged.subspan(offset, bytes).data(), bytes);
  return {};
}

/**
 * @brief Writes completed coordinates after releasing compute buffers and their reservation.
 * @param work Supplies cached or temporary coordinates and the preferred publication page size.
 * @param vertices Receives XYZ tuples in order; earlier writes remain if a later operation fails.
 * @param terminalDelta Takes ownership of the completed delta buffer for cached-coordinate byte staging.
 * @return Success with I/O warnings, or a staging, admission, read, or write error.
 * @note Releasing compute admission lets the output store reserve its copy-on-write buffers.
 * After byte staging, terminalDelta provides storage only. Do not read or resize its double elements.
 * Publication does not check cancellation. Completed passes can still be published after cancellation.
 */
Result<> publishSmoothedCoordinates(SmoothingWorkspace& work, Float32AbstractDataStore& vertices, std::vector<double> terminalDelta)
{
  const usize vertexCount = vertices.getNumberOfTuples();
  constexpr usize maximum = std::numeric_limits<usize>::max();
  if(vertexCount == 0)
  {
    releaseSmoothingCompute(work);
    return {};
  }
  if(vertexCount > maximum / (3 * sizeof(float32)) || terminalDelta.size() > maximum / sizeof(double))
  {
    return MakeErrorResult(-574, "Completed smoothing coordinates exceed the addressable terminal byte range.");
  }
  const usize coordinateBytes = vertexCount * 3 * sizeof(float32);
  std::span<const std::byte> staged;
  if(work.CacheVertices)
  {
    auto storage = std::as_writable_bytes(std::span<double>(terminalDelta));
    if(work.Vertices.size() != vertexCount * 3 || coordinateBytes > storage.size())
    {
      return MakeErrorResult(-574, "Completed smoothing coordinates do not fit the existing terminal delta storage.");
    }
    std::memcpy(storage.data(), work.Vertices.data(), coordinateBytes);
    staged = storage.first(coordinateBytes);
  }
  const usize preferredCapacity = work.PublicationCapacity;
  releaseSmoothingCompute(work);
  auto& budget = CacheMemoryBudgetManager::instance();
  Result<> result;
  {
    auto firstReservation = budget.reserveWorkingMemory(3 * sizeof(float32));
    if(firstReservation.sizeBytes() < 3 * sizeof(float32))
    {
      return MakeErrorResult(-575, fmt::format("Smoothing publication needs {} working bytes for its first XYZ tuple, but received {}.", 3 * sizeof(float32), firstReservation.sizeBytes()));
    }
    std::array<float32, 3> first{};
    result = MergeResults(std::move(result), readPublicationCoordinates(work.Temporary.get(), staged, 0, {first.data(), first.size()}));
    if(result.invalid())
    {
      return result;
    }
    // This real first output tuple permits an imported store to reserve its own COW buffers.
    result = MergeResults(std::move(result), vertices.copyFromBuffer(0, {first.data(), first.size()}));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-563, "Error writing the first smoothed XYZ tuple."));
    }
  }
  if(vertexCount == 1)
  {
    return result;
  }
  auto reservation = budget.reserveWorkingMemory(std::min<uint64>(preferredCapacity * 3 * sizeof(float32), budget.maximumWorkingMemoryBytes()));
  const usize capacity = static_cast<usize>(std::min<uint64>(preferredCapacity, reservation.sizeBytes() / (3 * sizeof(float32))));
  if(capacity == 0)
  {
    return MergeResults(std::move(result),
                        MakeErrorResult(-575, fmt::format("Smoothing publication cannot admit one XYZ page after its first tuple. Received {} working bytes.", reservation.sizeBytes())));
  }
  std::vector<float32> page(capacity * 3);
  for(usize first = 1; first < vertexCount;)
  {
    const usize count = std::min(capacity, vertexCount - first);
    result = MergeResults(std::move(result), readPublicationCoordinates(work.Temporary.get(), staged, first, {page.data(), count * 3}));
    if(result.invalid())
    {
      return result;
    }
    result = MergeResults(std::move(result), vertices.copyFromBuffer(first * 3, {page.data(), count * 3}));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-563, fmt::format("Error writing smoothed XYZ tuples at vertex {} ({} vertices).", first, count)));
    }
    first += count;
  }
  return result;
}

Result<> smoothOutOfCoreMesh(DataStructure& dataStructure, const LaplacianSmoothingInputValues& input, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messages)
{
  Result<> result;
  try
  {
    auto& geometry = dataStructure.getDataRefAs<INodeGeometry1D>(input.pTriangleGeometryDataPath);
    auto& vertices = geometry.getVertices()->getDataStoreRef();
    auto& nodes = dataStructure.getDataRefAs<Int8Array>(input.pSurfaceMeshNodeTypeArrayPath).getDataStoreRef();
    const usize vertexCount = vertices.getNumberOfTuples();
    constexpr usize maximum = std::numeric_limits<usize>::max();
    if(vertices.getNumberOfComponents() != 3 || nodes.getNumberOfComponents() != 1 || nodes.getNumberOfTuples() != vertexCount || vertexCount > maximum / (3 * sizeof(double)))
    {
      return MakeErrorResult(-576, "Smoothing requires addressable XYZ vertices and one node classification per vertex. Repair the array shapes or reduce the mesh size.");
    }
    std::vector<float32> lambdas;
    result = MergeResults(std::move(result), readSmoothingLambdas(nodes, lambdas, input));
    if(result.invalid())
    {
      return result;
    }
    if(auto* trianglesPtr = dataStructure.getDataAs<INodeGeometry2D>(input.pTriangleGeometryDataPath); trianglesPtr != nullptr)
    {
      result = MergeResults(std::move(result), trianglesPtr->findEdges(false));
      if(result.invalid())
      {
        return result;
      }
    }
    auto* edgeArrayPtr = geometry.getEdges();
    if(edgeArrayPtr == nullptr || edgeArrayPtr->getNumberOfComponents() != 2)
    {
      return MergeResults(std::move(result), MakeErrorResult(-577, "Smoothing requires a two-component edge array. Supply or generate valid mesh edges."));
    }
    auto& edges = edgeArrayPtr->getDataStoreRef();
    SmoothingWorkspace work;
    result = MergeResults(std::move(result), prepareSmoothingWorkspace(work, vertices, edges));
    if(result.invalid())
    {
      return result;
    }
    // These older arrays remain about 32 bytes per vertex with lambdas. This cache correction does not bound the complete smoother.
    std::vector<int32> connections(vertexCount, 0);
    std::vector<double> deltas(vertexCount * 3, 0.0);
    bool modified = false;
    ProgressEstimator progressEstimator;
    progressEstimator.restart();
    const usize iterationCount = input.pIterationSteps > 0 ? static_cast<usize>(input.pIterationSteps) : 0;
    messages.sendProgressCount("Smoothing Iterations", 0, iterationCount);
    for(int32 iteration = 0; iteration < input.pIterationSteps; ++iteration)
    {
      if(shouldCancel)
      {
        break;
      }
      result = MergeResults(std::move(result), accumulateSmoothingDeltas(work, edges, vertexCount, deltas, connections));
      if(result.invalid())
      {
        return result;
      }
      result = MergeResults(std::move(result), updateSmoothingCoordinates(work, lambdas, deltas, connections, false, input.pMuFactor));
      if(result.invalid())
      {
        return result;
      }
      modified = true;
      if(input.pUseTaubinSmoothing)
      {
        messages.sendProgressCount("Smoothing Iterations", static_cast<usize>(iteration), iterationCount, "Starting Taubin pass");
        if(shouldCancel)
        {
          break;
        }
        result = MergeResults(std::move(result), accumulateSmoothingDeltas(work, edges, vertexCount, deltas, connections));
        if(result.invalid())
        {
          return result;
        }
        result = MergeResults(std::move(result), updateSmoothingCoordinates(work, lambdas, deltas, connections, true, input.pMuFactor));
        if(result.invalid())
        {
          return result;
        }
      }
      messages.sendProgressCount("Smoothing Iterations", static_cast<usize>(iteration) + 1, iterationCount, progressEstimator.estimate(static_cast<usize>(iteration) + 1, iterationCount));
    }
    if(modified)
    {
      result = MergeResults(std::move(result), publishSmoothedCoordinates(work, vertices, std::move(deltas)));
    }
    return result;
  } catch(const std::bad_alloc&)
  {
    return MergeResults(std::move(result), MakeErrorResult(-578, fmt::format("Smoothing could not allocate its working buffers for '{}'. Free memory or reduce the mesh size.",
                                                                             input.pTriangleGeometryDataPath.toString())));
  } catch(const std::length_error&)
  {
    return MergeResults(std::move(result),
                        MakeErrorResult(-578, fmt::format("Smoothing exceeded an addressable buffer size for '{}'. Reduce the mesh size.", input.pTriangleGeometryDataPath.toString())));
  }
}
} // namespace

LaplacianSmoothing::LaplacianSmoothing(DataStructure& dataStructure, LaplacianSmoothingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

LaplacianSmoothing::~LaplacianSmoothing() noexcept = default;

Result<> LaplacianSmoothing::operator()()
{
  auto result = edgeBasedSmoothing();

  // At the end of the algorithm, the 2D Node Geometry will have edges that are not
  // needed. This will remove those from the DataStructure
  if(auto* nodeGeom2DPtr = m_DataStructure.getDataAs<INodeGeometry2D>(m_InputValues->pTriangleGeometryDataPath); nullptr != nodeGeom2DPtr)
  {
    auto edgeListId = nodeGeom2DPtr->getEdgeListId();
    nodeGeom2DPtr->setEdgeListId(0);
    auto edgeDataId = nodeGeom2DPtr->getEdgeListDataArrayId();
    nodeGeom2DPtr->setEdgeDataId(0);
    m_DataStructure.removeData(edgeListId);
    m_DataStructure.removeData(edgeDataId);
  }
  return result;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> LaplacianSmoothing::edgeBasedSmoothing()
{
  auto& nodeGeom1DRef = m_DataStructure.getDataRefAs<INodeGeometry1D>(m_InputValues->pTriangleGeometryDataPath);

  if(nodeGeom1DRef.getVertices() == nullptr)
  {
    return MakeErrorResult(-559, fmt::format("Error finding vertices for geometry at path '{}'.", m_InputValues->pTriangleGeometryDataPath.toString()));
  }

  Float32AbstractDataStore& vertDataStoreRef = nodeGeom1DRef.getVertices()->getDataStoreRef();
  IGeometry::MeshIndexType numberOfVertices = nodeGeom1DRef.getNumberOfVertices();

  const auto* surfacePtr = m_DataStructure.getDataAs<INodeGeometry2D>(m_InputValues->pTriangleGeometryDataPath);
  const bool usesOutOfCoreStore = AnyOutOfCore({nodeGeom1DRef.getVertices(), nodeGeom1DRef.getEdges(), surfacePtr == nullptr ? nullptr : surfacePtr->getFaces(),
                                                m_DataStructure.getDataAs<Int8Array>(m_InputValues->pSurfaceMeshNodeTypeArrayPath)});
  const bool useOutOfCoreAlgorithm = ShouldUseOutOfCoreAlgorithm(usesOutOfCoreStore);
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    return smoothOutOfCoreMesh(m_DataStructure, *m_InputValues, m_ShouldCancel, m_MessageHandler);
  }

  // Generate the Lambda Array
  std::vector<float> lambdas;
  Result<> result = generateLambdaArray(lambdas);
  if(result.invalid())
  {
    return result;
  }

  auto inode2DPtr = m_DataStructure.getDataAs<INodeGeometry2D>(m_InputValues->pTriangleGeometryDataPath);
  if(nullptr != inode2DPtr)
  {
    //  Generate the Unique Edges
    result = MergeResults(std::move(result), inode2DPtr->findEdges(false));
    if(result.invalid())
    {
      return result;
    }
  }

  AbstractDataStore<IGeometry::SharedEdgeList::value_type>& edgeDataStore = nodeGeom1DRef.getEdges()->getDataStoreRef();
  IGeometry::MeshIndexType numEdges = edgeDataStore.getNumberOfTuples();

  // Cache coordinates and edges once to avoid per-element OOC I/O in every iteration.
  // The edge list adds 2 * numEdges * sizeof(MeshIndexType) bytes; scratch memory remains O(mesh size), like the existing vertex arrays.
  std::vector<float> vertices(numberOfVertices * 3);
  result = MergeResults(std::move(result), vertDataStoreRef.copyIntoBuffer(0, nonstd::span<float>(vertices.data(), vertices.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-561, fmt::format("Error reading vertices for geometry at path '{}'.", m_InputValues->pTriangleGeometryDataPath.toString())));
  }

  std::vector<IGeometry::MeshIndexType> edges(numEdges * 2);
  result = MergeResults(std::move(result), edgeDataStore.copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(edges.data(), edges.size())));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-562, fmt::format("Error reading edges for geometry at path '{}'.", m_InputValues->pTriangleGeometryDataPath.toString())));
  }

  bool verticesModified = false;
  // Persist completed passes on cancellation, at the same checkpoints as the original algorithm.
  const auto writeVertices = [&]() -> Result<> {
    if(verticesModified)
    {
      result = MergeResults(std::move(result), vertDataStoreRef.copyFromBuffer(0, nonstd::span<const float>(vertices.data(), vertices.size())));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-563, fmt::format("Error writing smoothed vertices for geometry at path '{}'.", m_InputValues->pTriangleGeometryDataPath.toString())));
      }
    }
    return std::move(result);
  };

  std::vector<int32> numConnections(numberOfVertices, 0);

  std::vector<double> deltaArray(numberOfVertices * 3);
  double dlta = 0.0;

  ProgressEstimator progressEstimator;
  progressEstimator.restart();
  const usize iterationCount = m_InputValues->pIterationSteps > 0 ? static_cast<usize>(m_InputValues->pIterationSteps) : 0;
  m_MessageHandler.sendProgressCount("Smoothing Iterations", 0, iterationCount);
  for(int32_t q = 0; q < m_InputValues->pIterationSteps; q++)
  {
    if(m_ShouldCancel)
    {
      return writeVertices();
    }
    // Compute the Deltas for each point
    for(IGeometry::MeshIndexType i = 0; i < numEdges; i++)
    {
      IGeometry::MeshIndexType in1 = edges[2 * i];     // row of the first vertex
      IGeometry::MeshIndexType in2 = edges[2 * i + 1]; // row the second vertex

      for(IGeometry::MeshIndexType j = 0; j < 3; j++)
      {
#if 0
        Q_ASSERT(static_cast<size_t>(3 * in1 + j) < static_cast<size_t>(numberOfVertices * 3));
        Q_ASSERT(static_cast<size_t>(3 * in2 + j) < static_cast<size_t>(numberOfVertices * 3));
#endif
        dlta = static_cast<double>(vertices[3 * in2 + j] - vertices[3 * in1 + j]);
        deltaArray[3 * in1 + j] += dlta;
        deltaArray[3 * in2 + j] += -1.0 * dlta;
      }
      numConnections[in1] += 1;
      numConnections[in2] += 1;
    }

    // Move each point
    for(IGeometry::MeshIndexType i = 0; i < numberOfVertices; i++)
    {
      for(IGeometry::MeshIndexType j = 0; j < 3; j++)
      {
        IGeometry::MeshIndexType in0 = 3 * i + j;
        dlta = deltaArray[in0] / numConnections[i];

        float ll = lambdas[i];
        // ValueProxy::operator+= converts the displacement to float before the addition.
        vertices[3 * i + j] += static_cast<float>(ll * dlta);
        deltaArray[in0] = 0.0; // reset for next iteration
      }
      numConnections[i] = 0; // reset for next iteration
    }
    verticesModified = true;

    // Now optionally apply a negative lambda based on the mu Factor value.
    // This is from Taubin's paper on smoothing without shrinkage. This effectively
    // runs a low pass filter on the data
    if(m_InputValues->pUseTaubinSmoothing)
    {
      m_MessageHandler.sendProgressCount("Smoothing Iterations", static_cast<usize>(q), iterationCount, "Starting Taubin pass");
      if(m_ShouldCancel)
      {
        return writeVertices();
      }
      // Compute the Delta's
      for(IGeometry::MeshIndexType i = 0; i < numEdges; i++)
      {
        IGeometry::MeshIndexType in1 = edges[2 * i];     // row of the first vertex
        IGeometry::MeshIndexType in2 = edges[2 * i + 1]; // row the second vertex

        for(int32_t j = 0; j < 3; j++)
        {
#if 0
          Q_ASSERT(static_cast<size_t>(3 * in1 + j) < static_cast<size_t>(numberOfVertices * 3));
          Q_ASSERT(static_cast<size_t>(3 * in2 + j) < static_cast<size_t>(numberOfVertices * 3));
#endif
          dlta = vertices[3 * in2 + j] - vertices[3 * in1 + j];
          deltaArray[3 * in1 + j] += dlta;
          deltaArray[3 * in2 + j] += -1.0 * dlta;
        }
        numConnections[in1] += 1;
        numConnections[in2] += 1;
      }

      // Move the points
      for(IGeometry::MeshIndexType i = 0; i < numberOfVertices; i++)
      {
        for(IGeometry::MeshIndexType j = 0; j < 3; j++)
        {
          IGeometry::MeshIndexType in0 = 3 * i + j;
          dlta = deltaArray[in0] / numConnections[i];

          float ll = lambdas[i] * m_InputValues->pMuFactor;
          vertices[3 * i + j] += static_cast<float>(ll * dlta);
          deltaArray[in0] = 0.0; // reset for next iteration
        }
        numConnections[i] = 0; // reset for next iteration
      }
    }
    m_MessageHandler.sendProgressCount("Smoothing Iterations", static_cast<usize>(q) + 1, iterationCount, progressEstimator.estimate(static_cast<usize>(q) + 1, iterationCount));
  }

  return writeVertices();
}

// -----------------------------------------------------------------------------
Result<> LaplacianSmoothing::generateLambdaArray(std::vector<float>& lambdas) const
{
  auto& surfaceMeshNode = m_DataStructure.getDataAs<Int8Array>(m_InputValues->pSurfaceMeshNodeTypeArrayPath)->getDataStoreRef();

  size_t numNodes = surfaceMeshNode.getNumberOfTuples();

  std::vector<int8> nodeTypes(numNodes);
  Result<> result = surfaceMeshNode.copyIntoBuffer(0, nonstd::span<int8>(nodeTypes.data(), nodeTypes.size()));
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(-560, fmt::format("Error reading node types at path '{}'.", m_InputValues->pSurfaceMeshNodeTypeArrayPath.toString())));
  }

  lambdas.assign(numNodes, 0.0f);

  for(size_t i = 0; i < numNodes; ++i)
  {
    switch(nodeTypes[i])
    {
    case nx::core::NodeType::Unused:
      break;
    case nx::core::NodeType::Default:
      lambdas[i] = m_InputValues->pLambda;
      break;
    case nx::core::NodeType::TriplePoint:
      lambdas[i] = m_InputValues->pTripleLineLambda;
      break;
    case nx::core::NodeType::QuadPoint:
      lambdas[i] = m_InputValues->pQuadPointLambda;
      break;
    case nx::core::NodeType::SurfaceDefault:
      lambdas[i] = m_InputValues->pSurfacePointLambda;
      break;
    case nx::core::NodeType::SurfaceTriplePoint:
      lambdas[i] = m_InputValues->pSurfaceTripleLineLambda;
      break;
    case nx::core::NodeType::SurfaceQuadPoint:
      lambdas[i] = m_InputValues->pSurfaceQuadPointLambda;
      break;
    default:
      break;
    }
  }

  return result;
}

// -----------------------------------------------------------------------------
// This is just here for some debugging issues.
// -----------------------------------------------------------------------------
#if 0
void LaplacianSmoothing::writeVTKFile(const QString& outputVtkFile)
{

  DataContainer::Pointer sm = getDataContainerArray()->getDataContainer(getSurfaceDataContainerName());  /* Place all your code to execute your filter here. */
  VertexArray& nodes = *(sm->getVertices());
  int nNodes = nodes.getNumberOfTuples();
  bool m_WriteBinaryFile = true;

  FILE* vtkFile = nullptr;
  vtkFile = fopen(outputVtkFile.toLatin1().data(), "wb");
  if (nullptr == vtkFile)
  {
    QString ss = QObject::tr("Error creating file '%1'").arg(outputVtkFile);
    setErrorCondition(-90123, ss);
    return;
  }
  Detail::ScopedFileMonitor vtkFileMonitor(vtkFile);

  fprintf(vtkFile, "# vtk DataFile Version 2.0\n");
  fprintf(vtkFile, "Data set from DREAM3D-NX Surface Meshing Module\n");
  if (m_WriteBinaryFile)
  {
    fprintf(vtkFile, "BINARY\n");
  }
  else
  {
    fprintf(vtkFile, "ASCII\n");
  }
  fprintf(vtkFile, "DATASET POLYDATA\n");


  fprintf(vtkFile, "POINTS %d float\n", nNodes);
  float pos[3] = {0.0f, 0.0f, 0.0f};

  size_t totalWritten = 0;
  // Write the POINTS data (Vertex)
  for (int i = 0; i < nNodes; i++)
  {
    VertexArray::Vert_t& n = nodes[i]; // Get the current Node
    //  if (m_SurfaceMeshNodeType[i] > 0)
    {
      pos[0] = static_cast<float>(n.pos[0]);
      pos[1] = static_cast<float>(n.pos[1]);
      pos[2] = static_cast<float>(n.pos[2]);
      if (m_WriteBinaryFile == true)
      {
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[0]);
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[1]);
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[2]);
        totalWritten = fwrite(pos, sizeof(float), 3, vtkFile);
        if (totalWritten != sizeof(float) * 3)
        {

        }
      }
      else
      {
        fprintf(vtkFile, "%4.4f %4.4f %4.4f\n", pos[0], pos[1], pos[2]); // Write the positions to the output file
      }
    }
  }

  // Write the triangle indices into the vtk File
  FaceArray& triangles = *(sm->getFaces());
  int triangleCount = 0;
  int end = triangles.getNumberOfTuples();
  int featureInterest = 9;
  for(int i = 0; i < end; ++i)
  {
    //FaceArray::Face_t* tri = triangles.getPointer(i);
    if (m_SurfaceMeshFaceLabels[i * 2] == featureInterest || m_SurfaceMeshFaceLabels[i * 2 + 1] == featureInterest)
    {
      ++triangleCount;
    }
  }


  int tData[4];
  // Write the CELLS Data
  //  int start = 3094380;
  //  int end = 3094450;
  //  int triangleCount = end - start;
  qDebug() << "---------------------------------------------------------------------------" << "\n";
  qDebug() << outputVtkFile << "\n";
  fprintf(vtkFile, "\nPOLYGONS %d %d\n", triangleCount, (triangleCount * 4));
  for (int tid = 0; tid < end; ++tid)
  {
    //FaceArray::Face_t* tri = triangles.getPointer(tid);
    if (m_SurfaceMeshFaceLabels[tid * 2] == featureInterest || m_SurfaceMeshFaceLabels[tid * 2 + 1] == featureInterest)
    {
      tData[1] = triangles[tid].verts[0];
      tData[2] = triangles[tid].verts[1];
      tData[3] = triangles[tid].verts[2];
      //      qDebug() << tid << "\n  " << nodes[tData[1]].coord[0] << " " << nodes[tData[1]].coord[1]  << " " << nodes[tData[1]].coord[2]  << "\n";
      //      qDebug() << "  " << nodes[tData[2]].coord[0] << " " << nodes[tData[2]].coord[1]  << " " << nodes[tData[2]].coord[2]  << "\n";
      //      qDebug() << "  " << nodes[tData[3]].coord[0] << " " << nodes[tData[3]].coord[1]  << " " << nodes[tData[3]].coord[2]  << "\n";
      if (m_WriteBinaryFile == true)
      {
        tData[0] = 3; // Push on the total number of entries for this entry
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[0]);
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[1]); // Index of Vertex 0
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[2]); // Index of Vertex 1
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[3]); // Index of Vertex 2
        fwrite(tData, sizeof(int), 4, vtkFile);
      }
      else
      {
        fprintf(vtkFile, "3 %d %d %d\n", tData[1], tData[2], tData[3]);
      }
    }
  }

  fprintf(vtkFile, "\n");
}
#endif
