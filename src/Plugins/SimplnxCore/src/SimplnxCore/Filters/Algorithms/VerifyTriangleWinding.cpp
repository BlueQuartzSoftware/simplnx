#include "VerifyTriangleWinding.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

namespace
{
using MeshIndexType = IGeometry::MeshIndexType;

// Keep the float32 operations and association identical to FindTriangleVolume.
float32 FindGatheredTriangleVolume(const float32* a, const float32* b, const float32* c)
{
  const std::array<float32, 9> matrix = {b[0] - a[0], c[0] - a[0], 0.0f - a[0], b[1] - a[1], c[1] - a[1], 0.0f - a[1], b[2] - a[2], c[2] - a[2], 0.0f - a[2]};
  const float32 determinant =
      (matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7])) - (matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6])) + (matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]));
  return determinant / 6.0f;
}

void AccumulateGatheredTriangleVolume(const float32* coordinates, const int32* ids, usize idComponents, std::vector<float32>& volumes)
{
  const float32* a = coordinates;
  const float32* b = coordinates + 3;
  const float32* c = coordinates + 6;
  const int32 first = ids[0];
  if(idComponents == 1)
  {
    if(first >= 0)
    {
      volumes[first] += FindGatheredTriangleVolume(a, c, b);
    }
    return;
  }
  const int32 second = ids[1];
  const bool firstInRange = first >= 0 && static_cast<usize>(first) < volumes.size();
  const bool secondInRange = second >= 0 && static_cast<usize>(second) < volumes.size();
  if(first < 0 && secondInRange)
  {
    volumes[second] += FindGatheredTriangleVolume(a, c, b);
  }
  else if(second < 0 && firstInRange)
  {
    volumes[first] += FindGatheredTriangleVolume(a, b, c);
  }
  else if(firstInRange && secondInRange)
  {
    volumes[first] += FindGatheredTriangleVolume(a, b, c);
    volumes[second] += FindGatheredTriangleVolume(a, c, b);
  }
}

bool MergeWindingTransfer(Result<>& result, Result<> transfer, const std::string& geometryPath, const char* phasePtr, usize firstTuple, usize tupleCount)
{
  if(transfer.invalid())
  {
    transfer = MergeResults(std::move(transfer), MakeErrorResult(-56325, fmt::format("Triangle winding for geometry '{}': {} failed at tuple {} ({} tuples). Check the preceding storage diagnostic.",
                                                                                     geometryPath, phasePtr, firstTuple, tupleCount)));
  }
  result = MergeResults(std::move(result), std::move(transfer));
  return result.valid();
}

/**
 * @brief Accumulates signed feature volumes and reverses selected faces through bounded bulk transfers.
 * @param triangles Supplies three vertex indexes per face and receives complete reversed tuples.
 * @param vertices Supplies three float32 coordinates per vertex.
 * @param idsStore Supplies one region ID or two face labels per face.
 * @param geometryPath Identifies the geometry in storage and allocation errors.
 * @param shouldCancel Stops work before the next transfer or bounded computation check.
 * @param messageHandler Receives phase announcements and throttled progress.
 * @return Storage warnings, or an error for invalid shapes, indexes, transfers, or insufficient working memory.
 * @note The working reservation bounds block buffers; the dense feature-volume array scales with the largest feature ID.
 * Cancellation discards incomplete volume sums. Completed reversal blocks remain written.
 */
Result<> ReverseNegativeVolumesBounded(AbstractDataStore<MeshIndexType>& triangles, const Float32AbstractDataStore& vertices, const Int32AbstractDataStore& idsStore, const std::string& geometryPath,
                                       const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  Result<> result;
  const usize triangleCount = triangles.getNumberOfTuples();
  const usize vertexCount = vertices.getNumberOfTuples();
  const usize idComponents = idsStore.getNumberOfComponents();
  if(shouldCancel || triangleCount == 0)
  {
    return result;
  }
  const usize maxSize = std::numeric_limits<usize>::max();
  if(triangles.getNumberOfComponents() != 3 || vertices.getNumberOfComponents() != 3 || (idComponents != 1 && idComponents != 2) || idsStore.getNumberOfTuples() != triangleCount ||
     triangleCount > maxSize / 3 || vertexCount > maxSize / 3)
  {
    return MakeErrorResult(-56322,
                           fmt::format("Triangle winding for geometry '{}': expected XYZ vertices, three corners per face, and one or two IDs per face. Found {} vertices with {} components, "
                                       "{} faces with {} components, and {} ID tuples with {} components. Correct the array shapes and ensure flat indexes fit in memory addressing.",
                                       geometryPath, vertexCount, vertices.getNumberOfComponents(), triangleCount, triangles.getNumberOfComponents(), idsStore.getNumberOfTuples(), idComponents));
  }

  // Face values, corner permutation, gathered XYZ, at most 3B vertex tuples, and IDs.
  // The existing dense feature accumulator is separate from this new bounded payload.
  const usize bytesPerTriangle = (3 * sizeof(MeshIndexType)) + (3 * sizeof(usize)) + (18 * sizeof(float32)) + (idComponents * sizeof(int32));
  auto& budget = CacheMemoryBudgetManager::instance();
  const usize requestedCapacity = static_cast<usize>(std::min<uint64>(std::min(triangleCount, maxSize / bytesPerTriangle), budget.maximumWorkingMemoryBytes() / bytesPerTriangle));
  const usize requestedBytes = requestedCapacity * bytesPerTriangle;
  auto reservation = budget.reserveWorkingMemory(requestedBytes);
  const usize blockCapacity = static_cast<usize>(std::min<uint64>(triangleCount, reservation.sizeBytes() / bytesPerTriangle));
  if(blockCapacity == 0)
  {
    return MakeErrorResult(-56323, fmt::format("Triangle winding for geometry '{}': requested {} working bytes and received {}; at least {} bytes are required for one triangle. "
                                               "Release other working reservations or increase the configured cache budget before retrying.",
                                               geometryPath, requestedBytes, reservation.sizeBytes(), bytesPerTriangle));
  }
  const usize pageCapacity = std::min(3 * blockCapacity, vertexCount);
  const char* phasePtr = "allocating bounded buffers";
  try
  {
    auto faces = std::make_unique<MeshIndexType[]>(3 * blockCapacity);
    auto cornerOrder = std::make_unique<usize[]>(3 * blockCapacity);
    auto gathered = std::make_unique<float32[]>(9 * blockCapacity);
    auto vertexPage = std::make_unique<float32[]>(3 * pageCapacity);
    auto ids = std::make_unique<int32[]>(idComponents * blockCapacity);
    ThrottledMessageHandler progressThrottle(messageHandler);
    phasePtr = "scanning feature IDs";
    messageHandler.sendInfoMessage("Scanning feature IDs");
    progressThrottle.reset(triangleCount, "Scanning feature IDs");
    int32 maxFeature = 0;
    for(usize begin = 0; begin < triangleCount;)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = std::min(blockCapacity, triangleCount - begin);
      if(!MergeWindingTransfer(result, idsStore.copyIntoBuffer(begin * idComponents, {ids.get(), count * idComponents}), geometryPath, phasePtr, begin, count))
      {
        return result;
      }
      for(usize index = 0; index < count * idComponents; ++index)
      {
        maxFeature = std::max(maxFeature, ids[index]);
      }
      begin += count;
      progressThrottle.updatePercent(begin);
    }
    if(shouldCancel)
    {
      return result;
    }
    messageHandler.sendProgressPercent("Scanning feature IDs", triangleCount, triangleCount);
    phasePtr = "allocating feature volumes";
    const usize featureCount = static_cast<usize>(maxFeature) + 1;
    if(featureCount > maxSize / sizeof(float32))
    {
      return MergeResults(std::move(result),
                          MakeErrorResult(-56324, fmt::format("Triangle winding for geometry '{}': {} feature volumes exceed the addressable allocation size. Renumber sparse feature IDs.",
                                                              geometryPath, featureCount)));
    }
    std::vector<float32> volumes(featureCount, 0.0f);
    phasePtr = "calculating feature volumes";
    messageHandler.sendInfoMessage("Calculating feature volumes");
    progressThrottle.reset(triangleCount, "Calculating feature volumes");
    for(usize begin = 0; begin < triangleCount;)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = std::min(blockCapacity, triangleCount - begin);
      if(!MergeWindingTransfer(result, triangles.copyIntoBuffer(begin * 3, {faces.get(), count * 3}), geometryPath, "reading volume faces", begin, count) ||
         !MergeWindingTransfer(result, idsStore.copyIntoBuffer(begin * idComponents, {ids.get(), count * idComponents}), geometryPath, "reading volume IDs", begin, count))
      {
        return result;
      }
      for(usize corner = 0; corner < 3 * count; ++corner)
      {
        if(faces[corner] >= vertexCount)
        {
          return MergeResults(
              std::move(result),
              MakeErrorResult(-56326, fmt::format("Triangle winding for geometry '{}': face {} references vertex {}, but only {} vertices exist. Repair the face connectivity before retrying.",
                                                  geometryPath, begin + (corner / 3), faces[corner], vertexCount)));
        }
      }
      if(shouldCancel)
      {
        return result;
      }
      std::iota(cornerOrder.get(), cornerOrder.get() + (3 * count), usize{0});
      // Only I/O requests are sorted; feature additions retain their original order.
      // Cancellation waits for this bounded sort, then is checked before every page read.
      std::sort(cornerOrder.get(), cornerOrder.get() + (3 * count), [&faces](usize left, usize right) { return faces[left] < faces[right]; });
      usize loadedPage = maxSize;
      for(usize request = 0; request < 3 * count; ++request)
      {
        const usize corner = cornerOrder[request];
        const usize vertex = static_cast<usize>(faces[corner]);
        const usize pageBegin = (vertex / pageCapacity) * pageCapacity;
        if(pageBegin != loadedPage)
        {
          if(shouldCancel)
          {
            return result;
          }
          const usize pageCount = std::min(pageCapacity, vertexCount - pageBegin);
          if(!MergeWindingTransfer(result, vertices.copyIntoBuffer(pageBegin * 3, {vertexPage.get(), pageCount * 3}), geometryPath, "reading volume vertices", pageBegin, pageCount))
          {
            return result;
          }
          loadedPage = pageBegin;
        }
        std::copy_n(vertexPage.get() + ((vertex - pageBegin) * 3), 3, gathered.get() + (corner * 3));
      }
      for(usize triangle = 0; triangle < count; ++triangle)
      {
        if(triangle % 4096 == 0 && shouldCancel)
        {
          return result;
        }
        AccumulateGatheredTriangleVolume(gathered.get() + (triangle * 9), ids.get() + (triangle * idComponents), idComponents, volumes);
      }
      begin += count;
      progressThrottle.updatePercent(begin);
    }
    if(shouldCancel)
    {
      return result;
    }
    messageHandler.sendProgressPercent("Calculating feature volumes", triangleCount, triangleCount);
    if(shouldCancel)
    {
      return result;
    }
    if(std::none_of(volumes.begin() + 1, volumes.end(), [](float32 volume) { return volume < 0.0f; }))
    {
      messageHandler.sendInfoMessage("Triangle winding reversal complete");
      return result;
    }
    phasePtr = "reversing triangle winding";
    messageHandler.sendInfoMessage("Implementing reversal in bounded blocks");
    progressThrottle.reset(triangleCount, "Reversing triangle winding");
    for(usize begin = 0; begin < triangleCount;)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = std::min(blockCapacity, triangleCount - begin);
      if(!MergeWindingTransfer(result, triangles.copyIntoBuffer(begin * 3, {faces.get(), count * 3}), geometryPath, "reading reversal faces", begin, count) ||
         !MergeWindingTransfer(result, idsStore.copyIntoBuffer(begin * idComponents, {ids.get(), count * idComponents}), geometryPath, "reading reversal IDs", begin, count))
      {
        return result;
      }
      bool changed = false;
      for(usize triangle = 0; triangle < count; ++triangle)
      {
        if(triangle % 4096 == 0 && shouldCancel)
        {
          return result;
        }
        for(usize component = 0; component < idComponents; ++component)
        {
          const int32 id = ids[(triangle * idComponents) + component];
          if(id > 0 && static_cast<usize>(id) < volumes.size() && volumes[id] < 0.0f)
          {
            std::swap(faces[triangle * 3], faces[(triangle * 3) + 2]);
            changed = true;
            break;
          }
        }
      }
      if(shouldCancel)
      {
        return result;
      }
      if(changed && !MergeWindingTransfer(result, triangles.copyFromBuffer(begin * 3, {faces.get(), count * 3}), geometryPath, "writing reversed faces", begin, count))
      {
        return result;
      }
      begin += count;
      progressThrottle.updatePercent(begin);
    }
    messageHandler.sendProgressPercent("Reversing triangle winding", triangleCount, triangleCount);
    messageHandler.sendInfoMessage("Triangle winding reversal complete");
    return result;
  } catch(const std::bad_alloc&)
  {
    return MergeResults(
        std::move(result),
        MakeErrorResult(-56324, fmt::format("Triangle winding for geometry '{}': allocation failed while {} ({} reserved working bytes). Free memory or reduce sparse feature IDs before retrying.",
                                            geometryPath, phasePtr, reservation.sizeBytes())));
  } catch(const std::length_error&)
  {
    return MergeResults(
        std::move(result),
        MakeErrorResult(-56324, fmt::format("Triangle winding for geometry '{}': allocation length is unsupported while {}. Reduce the geometry size or renumber sparse feature IDs before retrying.",
                                            geometryPath, phasePtr)));
  }
}

class ReverseWindingImpl
{
public:
  using TriStore = AbstractDataStore<TriangleGeom::SharedFaceList::value_type>;
  ReverseWindingImpl(TriStore& triangles, const Int32AbstractDataStore& idsStore, const std::vector<usize>& reversalTargets, const std::atomic_bool& shouldCancel)
  : m_Triangles(triangles)
  , m_IdsStore(idsStore)
  , m_ReversalTargets(reversalTargets)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~ReverseWindingImpl() = default;

  void generateWithLabels(usize start, usize end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature1 = m_IdsStore[(i * 2) + 0];
      const int32 feature2 = m_IdsStore[(i * 2) + 1];
      for(const usize target : m_ReversalTargets)
      {
        if(target != feature1 && target != feature2)
        {
          continue;
        }

        // Flip it
        const IGeometry::MeshIndexType tempValue = m_Triangles[(i * 3) + 0];
        m_Triangles[(i * 3) + 0] = m_Triangles[(i * 3) + 2];
        m_Triangles[(i * 3) + 2] = tempValue;
        break;
      }
    }
  }

  void generateWithRegions(usize start, usize end) const
  {
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature = m_IdsStore[i];
      for(const usize target : m_ReversalTargets)
      {
        if(target != feature)
        {
          continue;
        }

        // Flip it
        const IGeometry::MeshIndexType tempValue = m_Triangles[(i * 3) + 0];
        m_Triangles[(i * 3) + 0] = m_Triangles[(i * 3) + 2];
        m_Triangles[(i * 3) + 2] = tempValue;
        break;
      }
    }
  }

  void operator()(const Range& range) const
  {
    if(m_IdsStore.getNumberOfComponents() == 2)
    {
      generateWithLabels(range.min(), range.max());
    }
    else if(m_IdsStore.getNumberOfComponents() == 1)
    {
      generateWithRegions(range.min(), range.max());
    }
  }

private:
  TriStore& m_Triangles;
  const Int32AbstractDataStore& m_IdsStore;
  const std::vector<usize>& m_ReversalTargets;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
VerifyTriangleWinding::VerifyTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             VerifyTriangleWindingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& VerifyTriangleWinding::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VerifyTriangleWinding::operator()()
{

  // Sort Vertices For Merging
  Result<> result = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath).validate();
  if(result.invalid())
  {
    return result;
  }

  m_MessageHandler.sendInfoMessage("Checking for duplicates - sorting vertices");
  const MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath), m_ShouldCancel);

  m_MessageHandler.sendInfoMessage("Checking for duplicates - validating");
  auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
  if(MeshingUtilities::HasDuplicateVertices(triGeom.getVertices()->getDataStoreRef(), sortedVerticesList))
  {
    // Remove duplicates
    return MakeErrorResult(-56320, "Duplicate vertices found in mesh, please use cleanup filters to flag and remove these before rerunning.");
  }

  TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();

  // Load mesh grouping (face labels or regions)
  const Int32AbstractDataStore& idsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->LabelsPath)->getDataStoreRef();

  // Scoped because we invalidate connectivity at the end
  Result<> windingResult = {};
  {
    // Generate Connectivity
    m_MessageHandler.sendInfoMessage("Generating Connectivity and Triangle Neighbors...");
    triGeom.findElementNeighbors(true);
    const auto optionalId = triGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56321, fmt::format("Unable to generate the connectivity list for {} geometry.", triGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    m_MessageHandler.sendInfoMessage("Repairing Windings...");
    // This is reused since it may contain warnings
    windingResult = MeshingUtilities::RepairTriangleWinding(triangles, connectivity, idsStore, m_ShouldCancel, m_MessageHandler);
    if(windingResult.invalid())
    {
      return windingResult;
    }

    // Purge connectivity
    m_DataStructure.removeData(triGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triGeom.getElementNeighborsId().value());
  }

  m_MessageHandler.sendInfoMessage("Assessing reversal - calculating feature volumes");
  const auto& vertices = triGeom.getVertices()->getDataStoreRef();
  const bool usesOutOfCoreStore =
      triangles.getStoreType() == IDataStore::StoreType::OutOfCore || vertices.getStoreType() == IDataStore::StoreType::OutOfCore || idsStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  const bool useOutOfCoreAlgorithm = ShouldUseOutOfCoreAlgorithm(usesOutOfCoreStore);
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    windingResult =
        MergeResults(std::move(windingResult), ReverseNegativeVolumesBounded(triangles, vertices, idsStore, m_InputValues->TargetGeometryPath.toString(), m_ShouldCancel, m_MessageHandler));
    if(windingResult.invalid() || m_ShouldCancel)
    {
      return windingResult;
    }
  }
  else
  {
    // Get max group (feature id != 0)
    int32 maxFeature = 0;
    for(int32 i = 0; i < idsStore.getSize(); i++)
    {
      maxFeature = std::max(idsStore[i], maxFeature);
    }

    std::vector<IGeometry::SharedVertexList::value_type> volumes(maxFeature + 1);
    auto volumeResult = MeshingUtilities::CalculateFeatureVolumes(triangles, triGeom.getVertices()->getDataStoreRef(), idsStore, volumes, m_ShouldCancel);
    if(volumeResult.invalid())
    {
      return volumeResult;
    }

    m_MessageHandler.sendInfoMessage("Implementing reversal in parallel");
    std::vector<usize> reversalTargets = {};
    reversalTargets.reserve(volumes.size());
    for(usize i = 1; i < volumes.size(); i++) // zero is an invalid feature label
    {
      if(volumes[i] < 0.0f)
      {
        reversalTargets.push_back(i);
      }
    }
    reversalTargets.shrink_to_fit();

    if(!reversalTargets.empty())
    {
      // Parallel algorithm to reverse based on feature label
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0ULL, triangles.getNumberOfTuples());
      dataAlg.execute(::ReverseWindingImpl(triangles, idsStore, reversalTargets, m_ShouldCancel));
    }
  }

  if(m_InputValues->RepairNormals)
  {
    m_MessageHandler.sendInfoMessage("Recalculating normals");
    auto& normals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->TriangleNormalsPath)->getDataStoreRef();

    windingResult = MergeResults(std::move(windingResult), MeshingUtilities::CalculateNormals(*triGeom.getFaces(), *triGeom.getVertices(), normals, m_ShouldCancel));
  }

  return windingResult;
}
