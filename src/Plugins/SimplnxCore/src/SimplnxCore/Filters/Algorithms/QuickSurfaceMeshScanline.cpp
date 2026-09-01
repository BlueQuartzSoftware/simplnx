/**
 * @file QuickSurfaceMeshScanline.cpp
 * @brief Implements scanline QuickSurfaceMesh execution for out-of-core (OOC) arrays.
 *
 * The algorithm holds two Feature ID slices and two node planes. Bulk I/O and
 * bounded temporary records avoid repeated load and eviction of disk-backed chunks.
 * The first pass resolves diagonal ambiguities. The second pass counts exact
 * output sizes. The final pass emits topology in per-slice batches.
 */

#include "QuickSurfaceMeshScanline.hpp"

#include "QuickSurfaceMesh.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
// Match QuickSurfaceMeshDirect random-number generator seed and call order for identical problem-voxel corrections.
constexpr float64 k_RangeMin = 0.0;
constexpr float64 k_RangeMax = 1.0;
constexpr std::mt19937_64::result_type k_Seed = 3412341234123412;
std::mt19937_64 generator(k_Seed);
std::uniform_real_distribution<> distribution(k_RangeMin, k_RangeMax);

// Fixed batches bound resident node-record memory independently of mesh size.
constexpr uint64 k_NodeRecordBatch = 4096;
constexpr usize k_NodeRecordPages = 16;

/**
 * @struct QuickSurfaceNodeRecord
 * @brief Stores external state for one generated mesh vertex.
 *
 * Bounded record pages hold coordinates and owner state instead of a mesh-sized
 * resident owner-list array.
 */
struct QuickSurfaceNodeRecord
{
  std::array<float32, 3> coordinates = {};
  std::array<int32, 4> owners = {}; // Holds up to four owner IDs because node types cap the count at four.
  uint8 ownerCount = 0;
  uint8 touchesExterior = 0;
  uint8 assigned = 0;
  uint8 reserved = 0; // Keeps the temporary record fixed-width.
};

/**
 * @brief Creates initialized temporary records for generated nodes.
 * @param nodeCount Number of node records to create.
 * @param allowInMemoryFallback True to permit resident temporary records.
 * @param shouldCancel Cancellation flag.
 * @return Owned node records or a provider, initialization, or cancellation error.
 *
 * OOC execution does not permit resident fallback. The restriction prevents node-state
 * scratch from exceeding RAM when the input cells use disk-backed storage.
 */
Result<std::unique_ptr<ITemporaryRecordStore>> CreateNodeRecordStore(uint64 nodeCount, bool allowInMemoryFallback, const std::atomic_bool& shouldCancel)
{
  if(nodeCount == 0)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-56342, "QuickSurfaceMesh cannot create node state for an empty mesh.");
  }
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(QuickSurfaceNodeRecord);
  config.maxRecordsPerBatch = k_NodeRecordBatch;
  config.initialRecordCount = nodeCount;
  auto result = DataStoreUtilities::GetIOCollection().createTemporaryRecordStore(config);
  if(result.invalid() && allowInMemoryFallback)
  {
    auto fallback = InMemoryTemporaryRecordStore::Create(config);
    if(fallback.invalid())
    {
      return ConvertInvalidResult<std::unique_ptr<ITemporaryRecordStore>>(std::move(fallback));
    }
    result = {std::move(fallback.value())};
  }
  if(result.invalid())
  {
    return result;
  }
  if(result.value() == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-56343, "QuickSurfaceMesh temporary-record provider returned a null node-state store.");
  }
  const QuickSurfaceNodeRecord emptyRecord;
  auto fillResult = result.value()->fill(0, nodeCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&emptyRecord), sizeof(emptyRecord)), shouldCancel);
  if(fillResult.invalid())
  {
    return ConvertInvalidResult<std::unique_ptr<ITemporaryRecordStore>>(std::move(fillResult));
  }
  return result;
}

/**
 * @class ExternalNodeRecords
 * @brief Adapts temporary node records for mesh generation.
 *
 * The adapter borrows the record store and cancellation flag. Its void mutators
 * capture I/O errors. Callers must return takeResult() when invalid() reports an error.
 */
class ExternalNodeRecords
{
public:
  /**
   * @class OwnerProxy
   * @brief Inserts owners for one node record.
   *
   * The proxy borrows its ExternalNodeRecords adapter. The adapter must outlive the proxy.
   */
  class OwnerProxy
  {
  public:
    /**
     * @brief Binds an adapter to one node record.
     * @param records Adapter that remains valid for the proxy lifetime.
     * @param nodeId Identifies the target node record.
     */
    OwnerProxy(ExternalNodeRecords& records, uint64 nodeId)
    : m_Records(records)
    , m_NodeId(nodeId)
    {
    }

    /**
     * @brief Records an owner and any exterior contact.
     * @param owner Feature owner. A value of -1 identifies exterior contact.
     */
    void insert(int32 owner)
    {
      m_Records.insert(m_NodeId, owner);
    }

  private:
    ExternalNodeRecords& m_Records;
    uint64 m_NodeId;
  };

  /**
   * @brief Binds temporary records to a fixed-size node-page cache.
   * @param store Record store that remains valid for the adapter lifetime.
   * @param shouldCancel Cancellation flag that remains valid for the adapter lifetime.
   */
  ExternalNodeRecords(ITemporaryRecordStore& store, const std::atomic_bool& shouldCancel)
  : m_Cache(store, k_NodeRecordBatch, k_NodeRecordPages)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Returns an owner-insertion proxy for one node record.
   * @param nodeId Identifies the target node record.
   * @return Proxy that borrows the ExternalNodeRecords adapter.
   */
  OwnerProxy operator[](uint64 nodeId)
  {
    return OwnerProxy(*this, nodeId);
  }

  /**
   * @brief Stores coordinates and marks a node as assigned.
   * @param nodeId Identifies the target node record.
   * @param coordinates Specifies the grid coordinates.
   *
   * The adapter captures cache errors for the caller to return after generation.
   */
  void setCoordinates(uint64 nodeId, const Point3D<float64>& coordinates)
  {
    if(m_ShouldCancel || m_Result.invalid())
    {
      return;
    }
    auto recordResult = m_Cache.read(nodeId, m_ShouldCancel);
    if(recordResult.invalid())
    {
      m_Result = ConvertResult(std::move(recordResult));
      return;
    }
    auto record = recordResult.value();
    record.coordinates = {static_cast<float32>(coordinates[0]), static_cast<float32>(coordinates[1]), static_cast<float32>(coordinates[2])};
    record.assigned = 1;
    m_Result = m_Cache.write(nodeId, record, m_ShouldCancel);
  }

  bool invalid() const
  {
    return m_Result.invalid();
  }

  /**
   * @brief Moves a captured I/O result to the caller.
   * @return Captured I/O error or a valid result.
   */
  Result<> takeResult()
  {
    return std::move(m_Result);
  }

  /**
   * @brief Flushes dirty node pages.
   * @return Captured I/O error, cache flush error, or a valid result.
   */
  Result<> flush()
  {
    if(m_Result.invalid())
    {
      return std::move(m_Result);
    }
    return m_Cache.flush(m_ShouldCancel);
  }

private:
  /**
   * @brief Updates exterior and unique-owner metadata through the bounded cache.
   * @param nodeId Identifies the target node record.
   * @param owner Feature owner to add. A value of -1 marks exterior contact.
   */
  void insert(uint64 nodeId, int32 owner)
  {
    if(m_ShouldCancel || m_Result.invalid())
    {
      return;
    }
    auto recordResult = m_Cache.read(nodeId, m_ShouldCancel);
    if(recordResult.invalid())
    {
      m_Result = ConvertResult(std::move(recordResult));
      return;
    }
    auto record = recordResult.value();
    if(owner == -1)
    {
      record.touchesExterior = 1;
    }
    const auto ownerEnd = record.owners.cbegin() + record.ownerCount;
    if(std::find(record.owners.cbegin(), ownerEnd, owner) == ownerEnd && record.ownerCount < record.owners.size())
    {
      record.owners[record.ownerCount++] = owner;
    }
    m_Result = m_Cache.write(nodeId, record, m_ShouldCancel);
  }

  BoundedRecordPageCache<QuickSurfaceNodeRecord> m_Cache;
  const std::atomic_bool& m_ShouldCancel;
  Result<> m_Result;
};

/**
 * @brief Resolves a body-diagonal problem-voxel case in the local slice buffer.
 * @param buf Contains local Feature IDs.
 * @param v1 First index in the Direct-compatible case order.
 * @param v2 Second index in the Direct-compatible case order.
 * @param v3 Third index in the Direct-compatible case order.
 * @param v4 Fourth index in the Direct-compatible case order.
 * @param v5 Fifth index in the Direct-compatible case order.
 * @param v6 Sixth index in the Direct-compatible case order.
 *
 * The choices and random-number sequence match QuickSurfaceMeshDirect. Only
 * the storage access changes from DataStore indexing to buffered values.
 */
void FlipProblemVoxelCase1(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3,
                           QuickSurfaceMeshScanline::MeshIndexType v4, QuickSurfaceMeshScanline::MeshIndexType v5, QuickSurfaceMeshScanline::MeshIndexType v6)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.25f)
  {
    buf[v6] = buf[v4];
  }
  else if(val < 0.5f)
  {
    buf[v6] = buf[v5];
  }
  else if(val < 0.75f)
  {
    buf[v1] = buf[v2];
  }
  else
  {
    buf[v1] = buf[v3];
  }
}

/**
 * @brief Resolves an edge-diagonal conflict in a local Feature ID buffer.
 * @param buf Contains local Feature IDs.
 * @param v1 First index in the Direct-compatible case order.
 * @param v2 Second index in the Direct-compatible case order.
 * @param v3 Third index in the Direct-compatible case order.
 * @param v4 Fourth index in the Direct-compatible case order.
 *
 * The random-number sequence and conditional order match QuickSurfaceMeshDirect.
 * Values below 0.375 perform an initial assignment before the independent below-0.5 branch.
 */
void FlipProblemVoxelCase2(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3,
                           QuickSurfaceMeshScanline::MeshIndexType v4)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.125f)
  {
    buf[v1] = buf[v2];
  }
  else if(val < 0.25f)
  {
    buf[v1] = buf[v3];
  }
  else if(val < 0.375f)
  {
    buf[v2] = buf[v1];
  }
  if(val < 0.5f)
  {
    buf[v2] = buf[v4];
  }
  else if(val < 0.625f)
  {
    buf[v3] = buf[v1];
  }
  else if(val < 0.75f)
  {
    buf[v3] = buf[v4];
  }
  else if(val < 0.875f)
  {
    buf[v4] = buf[v2];
  }
  else
  {
    buf[v4] = buf[v3];
  }
}

/**
 * @brief Resolves an isolated-voxel conflict in a local Feature ID buffer.
 * @param buf Contains local Feature IDs.
 * @param v1 First index in the Direct-compatible case order.
 * @param v2 Second index in the Direct-compatible case order.
 * @param v3 Third index in the Direct-compatible case order.
 *
 * The random-number sequence and two choices match QuickSurfaceMeshDirect.
 */
void FlipProblemVoxelCase3(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.5f)
  {
    buf[v2] = buf[v1];
  }
  else
  {
    buf[v3] = buf[v1];
  }
}
} // namespace

QuickSurfaceMeshScanline::QuickSurfaceMeshScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   const QuickSurfaceMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  generator.seed(k_Seed);
}

QuickSurfaceMeshScanline::~QuickSurfaceMeshScanline() noexcept = default;

Result<> QuickSurfaceMeshScanline::operator()()
{
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, false, m_ShouldCancel, m_MessageHandler);
  if(sentinelCheck.invalid())
  {
    return sentinelCheck;
  }

  auto& grid = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  SizeVec3 udims = grid.getDimensions();

  usize xP = udims[0];
  usize yP = udims[1];
  usize zP = udims[2];

  MeshIndexType nodeCount = 0;
  MeshIndexType triangleCount = 0;
  MeshIndexType suppressedFaceCount = 0;
  usize numFeatures = 0;

  // Correct diagonal ambiguities before the count pass determines exact output sizes.
  if(m_InputValues->FixProblemVoxels)
  {
    auto correctionResult = correctProblemVoxels();
    if(correctionResult.invalid())
    {
      return correctionResult;
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  auto countResult = countActiveNodesAndTriangles(nodeCount, triangleCount, numFeatures, suppressedFaceCount);
  if(countResult.invalid())
  {
    return countResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Resize after counting so generation writes contiguous, final-size output arrays.
  ShapeType tupleShape = {triangleCount};
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples(tupleShape);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({nodeCount});

  for(const auto& dataPath : m_InputValues->CreatedDataArrayPaths)
  {
    Result<> result = nx::core::ResizeAndReplaceDataArray(m_DataStructure, dataPath, tupleShape, nx::core::IDataAction::Mode::Execute);
    if(result.invalid())
    {
      return result;
    }
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && triangleCount == 0)
  {
    return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, featureIdsStore.getNumberOfTuples(), static_cast<usize>(nodeCount));
  }

  auto generationResult = createNodesAndTriangles(nodeCount, triangleCount, numFeatures);
  if(generationResult.invalid())
  {
    return generationResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    auto& ioCollection = DataStoreUtilities::GetIOCollection();
    // External sorting and temporary records repair winding without a mesh-sized resident adjacency structure.
    if(ioCollection.hasExternalSortCapability() && ioCollection.hasTemporaryRecordStoreCapability())
    {
      windingResult = MeshingUtilities::RepairTriangleWindingExternal(triangleGeom.getFaces()->getDataStoreRef(),
                                                                      m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);
    }
    else
    {
      const auto isOutOfCore = [](const IArray* array) { return array != nullptr && IsOutOfCore(*array); };
      bool hasOutOfCoreTarget =
          isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath)) || isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->NodeTypesDataPath)) ||
          isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FaceLabelsDataPath)) || isOutOfCore(triangleGeom.getVertices()) || isOutOfCore(triangleGeom.getFaces());
      const auto inspectPaths = [this, &hasOutOfCoreTarget, &isOutOfCore](const std::vector<DataPath>& paths) {
        for(const auto& path : paths)
        {
          hasOutOfCoreTarget = hasOutOfCoreTarget || isOutOfCore(m_DataStructure.getDataAs<IDataArray>(path));
        }
      };
      inspectPaths(m_InputValues->SelectedCellDataArrayPaths);
      inspectPaths(m_InputValues->SelectedFeatureDataArrayPaths);
      inspectPaths(m_InputValues->CreatedDataArrayPaths);
      if(hasOutOfCoreTarget)
      {
        return MakeErrorResult(
            -56344, "QuickSurfaceMesh cannot repair triangle winding for an out-of-core target because the active I/O provider does not support external sorting and temporary record stores.");
      }
      // In-memory targets can build transient adjacency when external sorting is unavailable.
      triangleGeom.findElementNeighbors(true);
      const auto optionalId = triangleGeom.getElementNeighborsId();
      if(!optionalId.has_value())
      {
        return MakeErrorResult(-56341, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
      }
      const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());
      windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                              m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);
      m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
      m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
    }
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && windingResult.valid())
  {
    if(triangleGeom.getNumberOfFaces() == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getNumberOfTuples(),
                                                    triangleGeom.getNumberOfVertices());
    }
    if(suppressedFaceCount == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(m_InputValues->TriangleGeometryPath);
    }
  }

  return windingResult;
}

Result<> QuickSurfaceMeshScanline::correctProblemVoxels()
{
  m_MessageHandler(IFilter::Message::Type::Info, "Correcting Problem Voxels");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;

  // Adjacent slice buffers bound correction memory to two Feature ID Z slices.
  auto sliceA = std::make_unique<int32[]>(sliceSize);
  auto sliceB = std::make_unique<int32[]>(sliceSize);

  MeshIndexType count = 1;
  MeshIndexType iter = 0;
  // Match the QuickSurfaceMeshDirect correction limit while repeated passes remove new conflicts.
  while(count > 0 && iter < 20)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    iter++;
    count = 0;

    for(MeshIndexType k = 1; k < zP; k++)
    {
      auto readResult = featureIdsStore.copyIntoBuffer((k - 1) * sliceSize, nonstd::span<int32>(sliceA.get(), sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }
      readResult = featureIdsStore.copyIntoBuffer(k * sliceSize, nonstd::span<int32>(sliceB.get(), sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }

      bool sliceADirty = false;
      bool sliceBDirty = false;

      for(MeshIndexType j = 1; j < yP; j++)
      {
        MeshIndexType row1 = (j - 1) * xP;
        MeshIndexType row2 = j * xP;
        for(MeshIndexType i = 1; i < xP; i++)
        {
          // Direct parity maps v1-v4 to sliceA (k-1) and v5-v8 to matching sliceB (k) offsets.
          MeshIndexType v1Local = row1 + i - 1;
          MeshIndexType v2Local = row1 + i;
          MeshIndexType v3Local = row2 + i - 1;
          MeshIndexType v4Local = row2 + i;

          int32 f1 = sliceA[v1Local];
          int32 f2 = sliceA[v2Local];
          int32 f3 = sliceA[v3Local];
          int32 f4 = sliceA[v4Local];
          int32 f5 = sliceB[v1Local];
          int32 f6 = sliceB[v2Local];
          int32 f7 = sliceB[v3Local];
          int32 f8 = sliceB[v4Local];

          // The inlined changes preserve QuickSurfaceMeshDirect case ordering and random-number draws. Dirty flags identify each changed slice.
          if(f1 == f8 && f1 != f2 && f1 != f3 && f1 != f4 && f1 != f5 && f1 != f6 && f1 != f7)
          {
            // FlipProblemVoxelCase1(featureIds, v1, v2, v3, v6, v7, v8)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v4Local] = sliceB[v2Local];
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v4Local] = sliceB[v3Local];
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v1Local] = sliceA[v2Local];
              sliceADirty = true;
            }
            else
            {
              sliceA[v1Local] = sliceA[v3Local];
              sliceADirty = true;
            }
            count++;
          }
          if(f2 == f7 && f2 != f1 && f2 != f3 && f2 != f4 && f2 != f5 && f2 != f6 && f2 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v2, v1, v4, v5, v8, v7)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v3Local] = sliceB[v1Local];
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v3Local] = sliceB[v4Local];
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v2Local] = sliceA[v1Local];
              sliceADirty = true;
            }
            else
            {
              sliceA[v2Local] = sliceA[v4Local];
              sliceADirty = true;
            }
            count++;
          }
          if(f3 == f6 && f3 != f1 && f3 != f2 && f3 != f4 && f3 != f5 && f3 != f7 && f3 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v3, v1, v4, v5, v8, v6)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v2Local] = sliceB[v1Local];
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v2Local] = sliceB[v4Local];
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v3Local] = sliceA[v1Local];
              sliceADirty = true;
            }
            else
            {
              sliceA[v3Local] = sliceA[v4Local];
              sliceADirty = true;
            }
            count++;
          }
          if(f4 == f5 && f4 != f1 && f4 != f2 && f4 != f3 && f4 != f6 && f4 != f7 && f4 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v4, v2, v3, v6, v7, v5)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v1Local] = sliceB[v2Local];
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v1Local] = sliceB[v3Local];
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v4Local] = sliceA[v2Local];
              sliceADirty = true;
            }
            else
            {
              sliceA[v4Local] = sliceA[v3Local];
              sliceADirty = true;
            }
            count++;
          }

          // Case 2 preserves the QuickSurfaceMeshDirect conditional sequence and marks each modified slice.
          auto doCase2 = [&](int32* bufX, MeshIndexType ix1, int32* bufY, MeshIndexType iy1, int32* bufZ, MeshIndexType iz1, int32* bufW, MeshIndexType iw1, bool& dirtyX, bool& dirtyY, bool& dirtyZ,
                             bool& dirtyW) {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.125f)
            {
              bufX[ix1] = bufY[iy1];
              dirtyX = true;
            }
            else if(val < 0.25f)
            {
              bufX[ix1] = bufZ[iz1];
              dirtyX = true;
            }
            else if(val < 0.375f)
            {
              bufY[iy1] = bufX[ix1];
              dirtyY = true;
            }
            if(val < 0.5f)
            {
              bufY[iy1] = bufW[iw1];
              dirtyY = true;
            }
            else if(val < 0.625f)
            {
              bufZ[iz1] = bufX[ix1];
              dirtyZ = true;
            }
            else if(val < 0.75f)
            {
              bufZ[iz1] = bufW[iw1];
              dirtyZ = true;
            }
            else if(val < 0.875f)
            {
              bufW[iw1] = bufY[iy1];
              dirtyW = true;
            }
            else
            {
              bufW[iw1] = bufZ[iz1];
              dirtyW = true;
            }
          };

          if(f1 == f6 && f1 != f2 && f1 != f5)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v2Local, sliceB.get(), v1Local, sliceB.get(), v2Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f2 == f5 && f2 != f1 && f2 != f6)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v1Local, sliceB.get(), v2Local, sliceB.get(), v1Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f3 == f8 && f3 != f4 && f3 != f7)
          {
            doCase2(sliceA.get(), v3Local, sliceA.get(), v4Local, sliceB.get(), v3Local, sliceB.get(), v4Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f4 == f7 && f4 != f3 && f4 != f8)
          {
            doCase2(sliceA.get(), v4Local, sliceA.get(), v3Local, sliceB.get(), v4Local, sliceB.get(), v3Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f1 == f7 && f1 != f3 && f1 != f5)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v3Local, sliceB.get(), v1Local, sliceB.get(), v3Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f3 == f5 && f3 != f1 && f3 != f7)
          {
            doCase2(sliceA.get(), v3Local, sliceA.get(), v1Local, sliceB.get(), v3Local, sliceB.get(), v1Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f2 == f8 && f2 != f4 && f2 != f6)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v4Local, sliceB.get(), v2Local, sliceB.get(), v4Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f4 == f6 && f4 != f2 && f4 != f8)
          {
            doCase2(sliceA.get(), v4Local, sliceA.get(), v2Local, sliceB.get(), v4Local, sliceB.get(), v2Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          // Same-plane Case 2 variants modify only one slice.
          if(f1 == f4 && f1 != f2 && f1 != f3)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v2Local, sliceA.get(), v3Local, sliceA.get(), v4Local, sliceADirty, sliceADirty, sliceADirty, sliceADirty);
            count++;
          }
          if(f2 == f3 && f2 != f1 && f2 != f4)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v1Local, sliceA.get(), v4Local, sliceA.get(), v3Local, sliceADirty, sliceADirty, sliceADirty, sliceADirty);
            count++;
          }
          if(f5 == f8 && f5 != f6 && f5 != f7)
          {
            doCase2(sliceB.get(), v1Local, sliceB.get(), v2Local, sliceB.get(), v3Local, sliceB.get(), v4Local, sliceBDirty, sliceBDirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f6 == f7 && f6 != f5 && f6 != f8)
          {
            doCase2(sliceB.get(), v2Local, sliceB.get(), v1Local, sliceB.get(), v4Local, sliceB.get(), v3Local, sliceBDirty, sliceBDirty, sliceBDirty, sliceBDirty);
            count++;
          }

          // Case 3 preserves the QuickSurfaceMeshDirect two-way choice and dirty-slice tracking.
          if(f2 == f3 && f2 == f4 && f2 == f5 && f2 == f6 && f2 == f7 && f2 != f1 && f2 != f8)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v1Local] = sliceA[v2Local];
              sliceADirty = true;
            }
            else
            {
              sliceB[v4Local] = sliceA[v2Local];
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f3 && f1 == f4 && f1 == f5 && f1 == f7 && f2 == f8 && f1 != f2 && f1 != f7)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v2Local] = sliceA[v1Local];
              sliceADirty = true;
            }
            else
            {
              sliceB[v3Local] = sliceA[v1Local];
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f2 && f1 == f4 && f1 == f5 && f1 == f7 && f1 == f8 && f1 != f3 && f1 != f6)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v3Local] = sliceA[v1Local];
              sliceADirty = true;
            }
            else
            {
              sliceB[v2Local] = sliceA[v1Local];
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f2 && f1 == f3 && f1 == f6 && f1 == f7 && f1 == f8 && f1 != f4 && f1 != f5)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v4Local] = sliceA[v1Local];
              sliceADirty = true;
            }
            else
            {
              sliceB[v1Local] = sliceA[v1Local];
              sliceBDirty = true;
            }
            count++;
          }
        }
      }

      // Dirty flags avoid writes for unchanged Feature ID slices.
      if(sliceADirty)
      {
        auto writeResult = featureIdsStore.copyFromBuffer((k - 1) * sliceSize, nonstd::span<const int32>(sliceA.get(), sliceSize));
        if(writeResult.invalid())
        {
          return writeResult;
        }
      }
      if(sliceBDirty)
      {
        auto writeResult = featureIdsStore.copyFromBuffer(k * sliceSize, nonstd::span<const int32>(sliceB.get(), sliceSize));
        if(writeResult.invalid())
        {
          return writeResult;
        }
      }
    }

    std::string ss = fmt::format("Correcting Problem Voxels: Iteration - '{}'; Problem Voxels - '{}'", iter, count);
    m_MessageHandler(IFilter::Message::Type::Info, ss);
  }
  return {};
}

// -----------------------------------------------------------------------------
/**
 * @brief Counting pass using rolling 2-plane node buffers and double-buffered
 * FeatureId Z-slices.
 *
 * This is the OOC equivalent of QuickSurfaceMeshDirect::determineActiveNodes().
 * The key difference is memory reduction: instead of an O(volume) nodeIds array,
 * this method uses two node-plane buffers of size O((xP+1)*(yP+1)) each.
 *
 * ## Rolling Buffer Strategy
 *
 * For Z-slice k, the dual-grid nodes lie on two planes:
 *   - nodePlane0: nodes at Z = k   (the "current" plane)
 *   - nodePlane1: nodes at Z = k+1 (the "next" plane)
 *
 * After processing all voxels in slice k:
 *   1. nodePlane0 is discarded (all its nodes have been assigned)
 *   2. nodePlane1 becomes nodePlane0 for the next iteration
 *   3. A fresh nodePlane1 is initialized with sentinel values
 *
 * This works because each node is referenced only by voxels at Z = k and Z = k-1.
 * Once we advance past Z = k, nodes in the Z = k plane are never accessed again.
 *
 * The FeatureIds are double-buffered similarly: curSlice holds Z = k, nextSlice
 * holds Z = k+1. After processing, they swap so the old next becomes current.
 *
 * Also tracks the maximum FeatureId value (numFeatures) for later array sizing.
 */
Result<> QuickSurfaceMeshScanline::countActiveNodesAndTriangles(MeshIndexType& nodeCount, MeshIndexType& triangleCount, usize& numFeatures, MeshIndexType& suppressedFaceCount)
{
  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;
  const MeshIndexType nodePlaneSize = (xP + 1) * (yP + 1);
  constexpr auto kMax = std::numeric_limits<MeshIndexType>::max();

  // Two planes keep node IDs proportional to one slice. Later cells cannot reference a retired Z plane.
  // The sentinel marks a node that needs an output ID.
  std::vector<MeshIndexType> nodePlane0(nodePlaneSize, kMax);
  std::vector<MeshIndexType> nodePlane1(nodePlaneSize, kMax);

  // Count each dual-grid node once before mesh output uses its ID.
  auto countNode = [&](std::vector<MeshIndexType>& plane, MeshIndexType offset) {
    if(plane[offset] == kMax)
    {
      plane[offset] = nodeCount;
      nodeCount++;
    }
  };

  // Two buffers supply the current and +Z neighbor slices through bulk I/O.
  auto curSlice = std::make_unique<int32[]>(sliceSize);
  auto nextSlice = std::make_unique<int32[]>(sliceSize);

  auto readResult = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.get(), sliceSize));
  if(readResult.invalid())
  {
    return readResult;
  }

  numFeatures = 0;

  for(MeshIndexType k = 0; k < zP; k++)
  {
    // Check cancellation per Z slice to keep the voxel loop free of atomic reads.
    if(m_ShouldCancel)
    {
      return {};
    }
    if(k < zP - 1)
    {
      readResult = featureIdsStore.copyIntoBuffer((k + 1) * sliceSize, nonstd::span<int32>(nextSlice.get(), sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }
    }

    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {
        MeshIndexType localIdx = j * xP + i;
        int32 curFeature = curSlice[localIdx];

        if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0)
        {
          suppressedFaceCount += static_cast<MeshIndexType>(i == 0) + static_cast<MeshIndexType>(i == xP - 1) + static_cast<MeshIndexType>(j == 0) + static_cast<MeshIndexType>(j == yP - 1) +
                                 static_cast<MeshIndexType>(k == 0) + static_cast<MeshIndexType>(k == zP - 1);
        }

        // Track max featureId for numFeatures
        if(static_cast<usize>(curFeature) > numFeatures)
        {
          numFeatures = static_cast<usize>(curFeature);
        }

        // A dual-grid node at (ni, nj) has offset nj * (xP + 1) + ni.

        if(i == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(j == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(k == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(i == (xP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(i < xP - 1 && curFeature != curSlice[localIdx + 1]) // neigh1 = point + 1
        {
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(j == (yP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(j < yP - 1 && curFeature != curSlice[localIdx + xP]) // neigh2 = point + xP
        {
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(k == (zP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(k < zP - 1 && curFeature != nextSlice[localIdx]) // neigh3 = point + xP*yP
        {
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
      }
    }

    // The next node plane becomes current. Reset the other plane for later nodes.
    std::swap(nodePlane0, nodePlane1);
    std::fill(nodePlane1.begin(), nodePlane1.end(), kMax);

    // The next Feature ID slice becomes current.
    std::swap(curSlice, nextSlice);
  }
  return {};
}

Result<> QuickSurfaceMeshScanline::createNodesAndTriangles(MeshIndexType nodeCount, MeshIndexType triangleCount, usize numFeatures)
{
  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Creating mesh");

  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;
  const MeshIndexType nodePlaneSize = (xP + 1) * (yP + 1);
  constexpr auto kMax = std::numeric_limits<MeshIndexType>::max();

  auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  ShapeType tDims = {nodeCount};
  triangleGeom->resizeVertexList(nodeCount);
  triangleGeom->resizeFaceList(triangleCount);
  triangleGeom->getFaceAttributeMatrix()->resizeTuples({triangleCount});
  triangleGeom->getVertexAttributeMatrix()->resizeTuples(tDims);

  auto& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();

  auto& nodeTypesStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypesStore.resizeTuples({nodeCount});

  VertexStore& vertex = triangleGeom->getVertices()->getDataStoreRef();
  TriStore& triangle = triangleGeom->getFaces()->getDataStoreRef();

  // A disk-backed source or output disallows resident node-record fallback.
  const auto isOutOfCore = [](const IArray* array) { return array != nullptr && IsOutOfCore(*array); };
  bool hasOutOfCoreTarget = isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath)) ||
                            isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->NodeTypesDataPath)) ||
                            isOutOfCore(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FaceLabelsDataPath)) || isOutOfCore(triangleGeom->getVertices()) || isOutOfCore(triangleGeom->getFaces());
  const auto inspectPaths = [this, &hasOutOfCoreTarget, &isOutOfCore](const std::vector<DataPath>& paths) {
    for(const auto& path : paths)
    {
      hasOutOfCoreTarget = hasOutOfCoreTarget || isOutOfCore(m_DataStructure.getDataAs<IDataArray>(path));
    }
  };
  inspectPaths(m_InputValues->SelectedCellDataArrayPaths);
  inspectPaths(m_InputValues->SelectedFeatureDataArrayPaths);
  inspectPaths(m_InputValues->CreatedDataArrayPaths);

  // OOC targets retain mesh-sized owner state externally instead of materializing it in RAM.
  auto nodeRecordStoreResult = CreateNodeRecordStore(nodeCount, !hasOutOfCoreTarget, m_ShouldCancel);
  if(nodeRecordStoreResult.invalid())
  {
    return ConvertResult(std::move(nodeRecordStoreResult));
  }
  auto nodeRecordStore = std::move(nodeRecordStoreResult.value());
  ExternalNodeRecords ownerLists(*nodeRecordStore, m_ShouldCancel);

  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    ::AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i + m_InputValues->SelectedCellDataArrayPaths.size()],
                                      m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  // Two Feature ID slices cover the +Z neighbor check without per-element store reads.
  auto curSlice = std::make_unique<int32[]>(sliceSize);
  auto nextSlice = std::make_unique<int32[]>(sliceSize);

  // Two node planes keep vertex IDs proportional to one Z slice.
  std::vector<MeshIndexType> nodePlane0(nodePlaneSize, kMax);
  std::vector<MeshIndexType> nodePlane1(nodePlaneSize, kMax);

  // Repeated face visits preserve QuickSurfaceMeshDirect last-write behavior. Bounded records hold owner state outside rolling planes.
  auto assignNode = [&](std::vector<MeshIndexType>& plane, MeshIndexType offset, MeshIndexType& assignedNodeCount, usize coordX, usize coordY, usize coordZ) {
    if(plane[offset] == kMax)
    {
      plane[offset] = assignedNodeCount;
      assignedNodeCount++;
    }
    nx::core::Point3D<float64> tmpCoords = grid->getPlaneCoords(coordX, coordY, coordZ);
    ownerLists.setCoordinates(plane[offset], tmpCoords);
  };

  auto featureReadResult = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.get(), sliceSize));
  if(featureReadResult.invalid())
  {
    return featureReadResult;
  }

  MeshIndexType triangleIndex = 0;
  MeshIndexType assignedNodeCount = 0;

  // Retain vector capacity to avoid allocation for each Z slice.
  std::vector<MeshIndexType> triBuffer;
  std::vector<int32> faceLabelBuf;
  std::vector<QuickSurfaceTransferData> ttArgsBuf;

  for(MeshIndexType k = 0; k < zP; k++)
  {
    // Check cancellation per Z slice before the next bulk read and mesh batch.
    if(m_ShouldCancel)
    {
      return {};
    }
    if(k < zP - 1)
    {
      featureReadResult = featureIdsStore.copyIntoBuffer((k + 1) * sliceSize, nonstd::span<int32>(nextSlice.get(), sliceSize));
      if(featureReadResult.invalid())
      {
        return featureReadResult;
      }
    }

    triBuffer.clear();
    faceLabelBuf.clear();
    ttArgsBuf.clear();

    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {
        MeshIndexType localIdx = j * xP + i;
        MeshIndexType point = k * sliceSize + localIdx;
        int32 curFeature = curSlice[localIdx];

        // A dual-grid node at (ni, nj) has offset nj * (xP + 1) + ni.

        if(i == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = j * (xP + 1) + i;
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(j == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = j * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + i;
          MeshIndexType n4Off = j * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(k == 0 && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = j * (xP + 1) + (i + 1);
          MeshIndexType n3Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n3Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane0, n4Off, assignedNodeCount, i + 1, j + 1, k);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane0[n3Off];
          MeshIndexType nid4 = nodePlane0[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(i == (xP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(i < xP - 1 && curFeature != curSlice[localIdx + 1])
        {
          int32 neigh1Feature = curSlice[localIdx + 1];
          MeshIndexType neigh1 = point + 1;

          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh1Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh1Feature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, curFeature, neigh1Feature});
          }
          else
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh1Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, neigh1Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh1Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh1Feature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, curFeature, neigh1Feature});
          }
          else
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh1Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, neigh1Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh1Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh1Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh1Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh1Feature);
        }
        if(j == (yP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(j < yP - 1 && curFeature != curSlice[localIdx + xP])
        {
          int32 neigh2Feature = curSlice[localIdx + xP];
          MeshIndexType neigh2 = point + xP;

          MeshIndexType n1Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh2Feature)
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh2Feature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, curFeature, neigh2Feature});
          }
          else
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(neigh2Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, neigh2Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh2Feature)
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh2Feature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, curFeature, neigh2Feature});
          }
          else
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(neigh2Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, neigh2Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh2Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh2Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh2Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh2Feature);
        }
        if(k == (zP - 1) && !(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && curFeature == 0))
        {
          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = j * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane1, n1Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n2Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane1[n1Off];
          MeshIndexType nid2 = nodePlane1[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(k < zP - 1 && curFeature != nextSlice[localIdx])
        {
          int32 neigh3Feature = nextSlice[localIdx];
          MeshIndexType neigh3 = point + sliceSize;

          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = j * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane1, n1Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n2Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane1[n1Off];
          MeshIndexType nid2 = nodePlane1[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh3Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh3Feature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, curFeature, neigh3Feature});
          }
          else
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh3Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, neigh3Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh3Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh3Feature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, curFeature, neigh3Feature});
          }
          else
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh3Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, neigh3Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh3Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh3Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh3Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh3Feature);
        }
      }
    }

    if(ownerLists.invalid())
    {
      return ownerLists.takeResult();
    }

    // Write each Z slice in contiguous batches to avoid per-face OOC I/O.
    if(!triBuffer.empty())
    {
      MeshIndexType sliceTriStart = triangleIndex - (triBuffer.size() / 3);
      auto triangleWriteResult = triangle.copyFromBuffer(sliceTriStart * 3, nonstd::span<const MeshIndexType>(triBuffer.data(), triBuffer.size()));
      if(triangleWriteResult.invalid())
      {
        return triangleWriteResult;
      }

      auto faceLabelsWriteResult = faceLabelsStore.copyFromBuffer(sliceTriStart * 2, nonstd::span<const int32>(faceLabelBuf.data(), faceLabelBuf.size()));
      if(faceLabelsWriteResult.invalid())
      {
        return faceLabelsWriteResult;
      }

      // Transfer all slice triangles together to avoid one virtual call per triangle.
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        auto transferResult = tupleTransferFunction->quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData>(ttArgsBuf.data(), ttArgsBuf.size()));
        if(transferResult.invalid())
        {
          return transferResult;
        }
      }
    }

    // The next node plane becomes current. Reset the other plane for later nodes.
    std::swap(nodePlane0, nodePlane1);
    std::fill(nodePlane1.begin(), nodePlane1.end(), kMax);

    // The next Feature ID slice becomes current.
    std::swap(curSlice, nextSlice);
  }

  auto nodeFlushResult = ownerLists.flush();
  if(nodeFlushResult.invalid())
  {
    return nodeFlushResult;
  }

  // Stream external node state in fixed contiguous batches. Fixed batches avoid a mesh-sized resident output buffer.
  auto recordBuffer = std::make_unique<QuickSurfaceNodeRecord[]>(k_NodeRecordBatch);
  auto coordinateBuffer = std::make_unique<VertexStore::value_type[]>(k_NodeRecordBatch * 3);
  auto nodeTypeBuffer = std::make_unique<int8[]>(k_NodeRecordBatch);
  for(uint64 recordOffset = 0; recordOffset < nodeCount; recordOffset += k_NodeRecordBatch)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const uint64 recordCount = std::min<uint64>(k_NodeRecordBatch, nodeCount - recordOffset);
    auto recordBytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(recordBuffer.get()), static_cast<usize>(recordCount) * sizeof(QuickSurfaceNodeRecord));
    auto readResult = nodeRecordStore->read(recordOffset, recordCount, recordBytes, m_ShouldCancel);
    if(readResult.invalid())
    {
      return ConvertResult(std::move(readResult));
    }
    if(readResult.value() != recordCount)
    {
      return MakeErrorResult(-56344, "QuickSurfaceMesh received a short read from its node-state store.");
    }
    // Match QuickSurfaceMeshDirect node types: capped owner count plus 10 for exterior contact.
    for(usize local = 0; local < recordCount; local++)
    {
      const auto& record = recordBuffer[local];
      if(record.assigned == 0)
      {
        return MakeErrorResult(-56345, "QuickSurfaceMesh encountered an unassigned node while streaming mesh output.");
      }
      coordinateBuffer[local * 3] = record.coordinates[0];
      coordinateBuffer[local * 3 + 1] = record.coordinates[1];
      coordinateBuffer[local * 3 + 2] = record.coordinates[2];
      nodeTypeBuffer[local] = static_cast<int8>(record.ownerCount + (record.touchesExterior != 0 ? 10 : 0));
    }
    auto vertexWriteResult = vertex.copyFromBuffer(static_cast<usize>(recordOffset) * 3, nonstd::span<const VertexStore::value_type>(coordinateBuffer.get(), static_cast<usize>(recordCount) * 3));
    if(vertexWriteResult.invalid())
    {
      return vertexWriteResult;
    }
    auto nodeTypeWriteResult = nodeTypesStore.copyFromBuffer(static_cast<usize>(recordOffset), nonstd::span<const int8>(nodeTypeBuffer.get(), static_cast<usize>(recordCount)));
    if(nodeTypeWriteResult.invalid())
    {
      return nodeTypeWriteResult;
    }
  }
  return {};
}
