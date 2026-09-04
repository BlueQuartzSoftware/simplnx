#include "M3CSurfaceMeshing.hpp"

#include "SimplnxCore/Filters/Algorithms/TupleTransfer.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>

#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include <algorithm>
#include <array>
#include <atomic>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <vector>

using namespace nx::core;

// The M3C core closely follows legacy DREAM3D M3CEntireVolume. Its flat,
// 1-based arrays preserve legacy topology and output ordering.
namespace
{
/**
 * @class TemporaryRecordVector
 * @brief Fixed-record scratch vector with a bounded typed page cache.
 * @tparam T Specifies the fixed scratch-record type.
 *
 * Candidate nodes, cube offsets, and triangle metadata use this wrapper. A
 * genuine OOC dispatch cannot fall back to resident scratch.
 */
template <typename T>
class TemporaryRecordVector
{
public:
  /**
   * @brief Creates the record store and its bounded typed cache.
   * @param recordCount Specifies initial fixed-record count.
   * @param requireExternalStore Prevents resident fallback during genuine OOC.
   * @param shouldCancel Stops before storage allocation when true.
   * @param recordsPerPage Specifies records per backing-store request.
   * @param cachePages Specifies maximum resident pages.
   * @return Initialized record vector, or a provider/allocation error.
   */
  static Result<TemporaryRecordVector> Create(uint64 recordCount, bool requireExternalStore, const std::atomic_bool& shouldCancel, uint64 recordsPerPage = 4096, usize cachePages = 8)
  {
    if(recordCount > 0 && shouldCancel)
    {
      return MakeErrorResult<TemporaryRecordVector>(-90540, "M3C temporary-record vector creation was cancelled.");
    }
    if(recordsPerPage == 0 || cachePages == 0 || recordsPerPage > std::numeric_limits<uint64>::max() / sizeof(T))
    {
      return MakeErrorResult<TemporaryRecordVector>(-90541, "M3C temporary-record vector configuration is invalid.");
    }

    TemporaryRecordStoreConfig config;
    config.recordSize = sizeof(T);
    config.maxRecordsPerBatch = recordsPerPage;
    config.initialRecordCount = recordCount;
    auto storeResult = DataStoreUtilities::GetIOCollection().createTemporaryRecordStore(config);

    std::unique_ptr<ITemporaryRecordStore> store;
    if(storeResult.valid())
    {
      store = std::move(storeResult.value());
    }
    else if(!requireExternalStore)
    {
      auto fallbackResult = InMemoryTemporaryRecordStore::Create(config);
      if(fallbackResult.invalid())
      {
        return ConvertInvalidResult<TemporaryRecordVector>(std::move(fallbackResult));
      }
      store = std::move(fallbackResult.value());
    }
    else
    {
      return ConvertInvalidResult<TemporaryRecordVector>(std::move(storeResult));
    }
    if(store == nullptr)
    {
      return MakeErrorResult<TemporaryRecordVector>(-90542, "M3C temporary-record provider returned a null store.");
    }

    TemporaryRecordVector vector;
    vector.m_Store = std::move(store);
    try
    {
      vector.m_Cache = std::make_unique<BoundedRecordPageCache<T>>(*vector.m_Store, recordsPerPage, cachePages);
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<TemporaryRecordVector>(-90543, "M3C temporary-record vector could not allocate its bounded page cache.");
    }
    return {std::move(vector)};
  }

  TemporaryRecordVector() = default;
  TemporaryRecordVector(TemporaryRecordVector&&) noexcept = default;
  TemporaryRecordVector& operator=(TemporaryRecordVector&&) noexcept = default;
  TemporaryRecordVector(const TemporaryRecordVector&) = delete;
  TemporaryRecordVector& operator=(const TemporaryRecordVector&) = delete;

  /**
   * @brief Returns the owned byte-record store.
   * @return Store used for bulk operations.
   */
  ITemporaryRecordStore& store() noexcept
  {
    return *m_Store;
  }

  /**
   * @brief Returns the owned typed page cache.
   * @return Cache used for localized random access.
   */
  BoundedRecordPageCache<T>& cache() noexcept
  {
    return *m_Cache;
  }

  /**
   * @brief Writes dirty pages before the next algorithm phase.
   * @param shouldCancel Stops cache flushing when true.
   * @return Error from cache flushing, or success.
   */
  Result<> flush(const std::atomic_bool& shouldCancel)
  {
    return m_Cache->flush(shouldCancel);
  }

private:
  std::unique_ptr<ITemporaryRecordStore> m_Store;
  std::unique_ptr<BoundedRecordPageCache<T>> m_Cache;
};

/**
 * @struct M3CCandidateNodeRecord
 * @brief External scratch record for one possible M3C node.
 * @p type records whether/how the candidate is used; @p pruneReferences records
 * whether dropped or surviving triangles reference it; @p compactId is assigned
 * after counting all live candidates so output vertices can be written densely.
 */
struct M3CCandidateNodeRecord
{
  int8 type = 0;
  uint8 pruneReferences = 0;
  std::array<std::byte, 6> padding{};
  uint64 compactId = 0;
};
static_assert(std::is_trivially_copyable_v<M3CCandidateNodeRecord>);

// A SiteId indexes the padded Feature Id grid. The 64-bit type prevents overflow
// when a large grid derives seven candidate-node IDs from each site.
using SiteId = int64;
// This sentinel marks candidate slots that are not real mesh nodes.
constexpr uint32 k_UnusedNodeId = std::numeric_limits<uint32>::max();

constexpr int num_neigh = 26;

struct Node
{
  float coord[3];
};
struct VoxelCoord
{
  float coord[3];
};
/**
 * @struct Neighbor
 * @brief Stores one-based indexes for 26 neighboring sites.
 */
struct Neighbor
{
  SiteId neigh_id[27]; // 1-based; index 0 unused. 64-bit: these index the FeatureId grid.
};
/**
 * @struct Face
 * @brief Stores one marching square's edges and center node.
 */
struct Face
{
  // Recompute corner sites to keep this largest working array compact. Edge IDs
  // are 32-bit mesh indexes. Face-center node IDs retain 64-bit site indexes.
  uint32 edge_id[4];
  SiteId FCnode; // face-center node id, -1 if none
  int8 nEdge;
  int8 effect; // 0 = useless square, 1 = straddles >=2 labels
};
/**
 * @struct Segment
 * @brief Stores one oriented face-edge segment and its labels.
 */
struct Segment
{
  int64 node_id[2];
  int nSpin[2]; // labels on left/right of the arrow
};
/**
 * @struct Triangle
 * @brief Stores one generated triangle and its adjacent labels.
 */
struct Triangle
{
  int64 node_id[3];
  int nSpin[2];
};

// Coordinates are pure functions of the padded site index. Compute them on
// demand to avoid full-volume coordinate arrays.
/**
 * @struct SiteCoords
 * @brief Calculates padded-grid coordinates on demand.
 */
struct SiteCoords
{
  usize fileDim0;
  usize fileDim1;
  usize fileNSP; // fileDim0 * fileDim1
  float res[3];
  float origin[3];

  /**
   * @brief Calculates one site coordinate.
   * @param site Specifies a one-based padded site index.
   * @return Coordinate in image units.
   */
  VoxelCoord operator[](int64 site) const
  {
    const usize linear = static_cast<usize>(site - 1);
    // Subtract the ghost shell so padded site (1,1,1) maps to the image origin.
    const int64 i = static_cast<int64>(linear % fileDim0) - 1;
    const int64 j = static_cast<int64>((linear / fileDim0) % fileDim1) - 1;
    const int64 k = static_cast<int64>(linear / fileNSP) - 1;
    // A site is a CELL CENTRE, not the cell's lower corner. initialize_nodes (legacy) places the 7
    // candidate nodes of a site at +half-spacing offsets, and the marching cube spans from one site
    // to its (+1,+1,+1) neighbour - so the interface between two adjacent cells falls on the plane
    // midway between their centres, which is exactly their shared face. Returning the lower corner
    // here instead shifted every vertex by half a cell, placing the mesh partly outside the volume.
    return VoxelCoord{{(static_cast<float>(i) + 0.5f) * res[0] + origin[0], (static_cast<float>(j) + 0.5f) * res[1] + origin[1], (static_cast<float>(k) + 0.5f) * res[2] + origin[2]}};
  }
};

/**
 * @struct NodeCoords
 * @brief Calculates seven M3C candidate-node coordinates per site.
 */
struct NodeCoords
{
  SiteCoords sites;

  /**
   * @brief Calculates one candidate-node coordinate.
   * @param id Specifies a zero-based candidate-node index.
   * @return Edge-midpoint, face-center, or body-center coordinate.
   *
   * Each site has three positive-edge midpoints, three positive-face centers,
   * and one body center. Their order matches the legacy node layout.
   */
  Node operator[](int64 id) const
  {
    const int64 site = id / 7 + 1;
    const int kind = static_cast<int>(id % 7);
    const VoxelCoord b = sites[site];
    const float hx = sites.res[0] / 2.0f;
    const float hy = sites.res[1] / 2.0f;
    const float hz = sites.res[2] / 2.0f;
    Node n{{b.coord[0], b.coord[1], b.coord[2]}};
    switch(kind)
    {
    case 0:
      n.coord[0] += hx;
      break;
    case 1:
      n.coord[1] += hy;
      break;
    case 2:
      n.coord[2] += hz;
      break;
    case 3:
      n.coord[0] += hx;
      n.coord[1] += hy;
      break;
    case 4:
      n.coord[0] += hx;
      n.coord[2] += hz;
      break;
    case 5:
      n.coord[1] += hy;
      n.coord[2] += hz;
      break;
    case 6:
      n.coord[0] += hx;
      n.coord[1] += hy;
      n.coord[2] += hz;
      break;
    default:
      break;
    }
    return n;
  }
};

// These 20 multi-material marching-square cases match the legacy slice and
// whole-volume algorithms. Each case contains at most four edges.
// k_EdgeTable2d maps node-slot pairs to edges. Slots 0 through 3 are edge
// midpoints, and slot 4 is the face center.
// k_NsTable2d maps each edge to the two corner labels on its sides.
// clang-format off
constexpr int k_EdgeTable2d[20][8] = {
    {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, {0, 1, -1, -1, -1, -1, -1, -1},   {-1, -1, -1, -1, -1, -1, -1, -1},
    {0, 2, -1, -1, -1, -1, -1, -1},   {1, 2, -1, -1, -1, -1, -1, -1},   {0, 4, 2, 4, 1, 4, -1, -1},       {-1, -1, -1, -1, -1, -1, -1, -1}, {3, 0, -1, -1, -1, -1, -1, -1},
    {3, 1, -1, -1, -1, -1, -1, -1},   {3, 4, 0, 4, 1, 4, -1, -1},       {2, 3, -1, -1, -1, -1, -1, -1},   {3, 4, 0, 4, 2, 4, -1, -1},       {3, 4, 1, 4, 2, 4, -1, -1},
    {3, 0, 1, 2, -1, -1, -1, -1},     {0, 1, 2, 3, -1, -1, -1, -1},     {0, 1, 2, 3, -1, -1, -1, -1},     {3, 0, 1, 2, -1, -1, -1, -1},     {3, 4, 1, 4, 0, 4, 2, 4}};

constexpr int k_NsTable2d[20][8] = {
    {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, {1, 0, -1, -1, -1, -1, -1, -1},   {-1, -1, -1, -1, -1, -1, -1, -1},
    {1, 0, -1, -1, -1, -1, -1, -1},   {2, 1, -1, -1, -1, -1, -1, -1},   {1, 0, 3, 2, 2, 1, -1, -1},       {-1, -1, -1, -1, -1, -1, -1, -1}, {0, 3, -1, -1, -1, -1, -1, -1},
    {0, 3, -1, -1, -1, -1, -1, -1},   {0, 3, 1, 0, 2, 1, -1, -1},       {3, 2, -1, -1, -1, -1, -1, -1},   {0, 3, 1, 0, 3, 2, -1, -1},       {0, 3, 2, 1, 3, 2, -1, -1},
    {0, 3, 2, 1, -1, -1, -1, -1},     {1, 0, 3, 2, -1, -1, -1, -1},     {1, 0, 3, 2, -1, -1, -1, -1},     {0, 3, 2, 1, -1, -1, -1, -1},     {0, 3, 2, 1, 1, 0, 3, 2}};
// clang-format on

// -----------------------------------------------------------------------------
// Copy FeatureIds into a 1-based working grid, wrapping it in a ghost shell of
// negative labels when addSurfaceLayer is true, fill voxel coordinates,
// and renumber any FeatureId==0 to maxGrainId. Returns maxGrainId (the value that
// zeros were remapped to; callers revert it on output). Transcribed from
// M3CEntireVolume::initialize_micro_from_grainIds.
// -----------------------------------------------------------------------------
// The single sentinel used for every cell of the ghost shell. Any negative value works; only the
// sign is tested. It must be the SAME for all ghost cells - see initialize_micro.
constexpr int32 k_GhostLabel = -3;

int initialize_micro(bool addSurfaceLayer, const usize dims[3], const usize fileDim[3], const AbstractDataStore<int32>& grainIds, int32* p)
{
  int maxGrainId = 0;

  if(!addSurfaceLayer)
  {
    usize totalPoints = dims[0] * dims[1] * dims[2];
    for(usize i = 0; i < totalPoints; ++i)
    {
      p[i + 1] = grainIds[i];
      if(p[i + 1] > maxGrainId)
      {
        maxGrainId = p[i + 1];
      }
    }
  }
  else
  {
    // Wrap the volume in a one-cell ghost shell. Ghost cells carry a NEGATIVE sentinel label so the
    // marching-cubes code treats them as "outside the volume"; only the sign is ever tested.
    //
    // Legacy used six DISTINCT sentinels here (-3 bottom z-slice, -4/-7 the y-row pads, -5/-6 the
    // per-row x-end pads, -8 top z-slice) to record which face or edge of the shell a ghost cell
    // belonged to. Nothing reads that back, but the marching cubes compares labels for INEQUALITY,
    // so neighbouring ghost cells with different sentinels looked like a material interface and were
    // triangulated - generating surface outside the volume along the shell's own internal seams.
    // A single shared sentinel leaves the shell internally uniform, so the only interfaces it can
    // produce are the real ghost-to-feature ones that form the volume's exterior surface.
    usize index = 0;
    usize gIdx = 0;

    // Bottom wrapping slice
    for(usize i = 0; i < (fileDim[0] * fileDim[1]); ++i)
    {
      p[++index] = k_GhostLabel;
    }
    // Bulk of the volume, wrapped per-plane and per-row
    for(usize z = 0; z < dims[2]; ++z)
    {
      for(usize i = 0; i < fileDim[0]; ++i)
      {
        p[++index] = k_GhostLabel;
      }
      for(usize y = 0; y < dims[1]; ++y)
      {
        p[++index] = k_GhostLabel; // leading surface voxel for this row
        for(usize x = 0; x < dims[0]; ++x)
        {
          p[++index] = grainIds[gIdx++];
          if(p[index] > maxGrainId)
          {
            maxGrainId = p[index];
          }
        }
        p[++index] = k_GhostLabel; // trailing surface voxel for this row
      }
      for(usize i = 0; i < fileDim[0]; ++i)
      {
        p[++index] = k_GhostLabel;
      }
    }
    // Top wrapping slice
    for(usize i = 0; i < (fileDim[0] * fileDim[1]); ++i)
    {
      p[++index] = k_GhostLabel;
    }
  }

  // Reserve one positive label for input Feature Id 0.
  maxGrainId = maxGrainId + 1;

  p[0] = 0; // Point 0 is garbage

  // Renumber zero labels without changing negative ghost cells. Coordinates are
  // computed on demand by SiteCoords and NodeCoords.
  const usize totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  for(usize id = 1; id <= totalPoints; id++)
  {
    if(p[id] == 0)
    {
      p[id] = maxGrainId;
    }
  }
  return maxGrainId;
}

/**
 * @struct NeighborAccessor
 * @brief Reconstructs 26 legacy neighbors for one padded site.
 *
 * The ghost shell makes toroidal border indexes harmless. Cache the returned
 * Neighbor when a caller needs multiple neighbor indexes.
 */
struct NeighborAccessor
{
  SiteId ns;
  SiteId nsp;
  int xDim;

  /**
   * @brief Calculates the 26 neighbors of one padded site.
   * @param site_id Specifies a one-based padded site index.
   * @return Neighbor indexes in the legacy order.
   */
  Neighbor operator[](SiteId site_id) const
  {
    // Recover the legacy loop coordinates for this one-based site.
    const SiteId within = (site_id - 1) % nsp;         // == j + (i - 1)
    const int i = static_cast<int>(within % xDim) + 1; // 1..xDim
    const SiteId j = within - (i - 1);                 // multiple of xDim, 0..nsp-xDim
    const SiteId k = ((site_id - 1) / nsp) * nsp;

    Neighbor n;
    n.neigh_id[0] = 0; // index 0 unused

    // same plane
    n.neigh_id[1] = k + j + i % xDim + 1;
    n.neigh_id[2] = k + (j - xDim + nsp) % nsp + i % xDim + 1;
    n.neigh_id[3] = k + (j - xDim + nsp) % nsp + i;
    n.neigh_id[4] = k + (j - xDim + nsp) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[5] = k + j + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[6] = k + (j + xDim) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[7] = k + (j + xDim) % nsp + i;
    n.neigh_id[8] = k + (j + xDim) % nsp + i % xDim + 1;

    // upper plane
    n.neigh_id[9] = (k - nsp + ns) % ns + j + i;
    n.neigh_id[10] = (k - nsp + ns) % ns + j + i % xDim + 1;
    n.neigh_id[11] = (k - nsp + ns) % ns + (j - xDim + nsp) % nsp + i % xDim + 1;
    n.neigh_id[12] = (k - nsp + ns) % ns + (j - xDim + nsp) % nsp + i;
    n.neigh_id[13] = (k - nsp + ns) % ns + (j - xDim + nsp) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[14] = (k - nsp + ns) % ns + j + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[15] = (k - nsp + ns) % ns + (j + xDim) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[16] = (k - nsp + ns) % ns + (j + xDim) % nsp + i;
    n.neigh_id[17] = (k - nsp + ns) % ns + (j + xDim) % nsp + i % xDim + 1;

    // lower plane
    n.neigh_id[18] = (k + nsp) % ns + j + i;
    n.neigh_id[19] = (k + nsp) % ns + j + i % xDim + 1;
    n.neigh_id[20] = (k + nsp) % ns + (j - xDim + nsp) % nsp + i % xDim + 1;
    n.neigh_id[21] = (k + nsp) % ns + (j - xDim + nsp) % nsp + i;
    n.neigh_id[22] = (k + nsp) % ns + (j - xDim + nsp) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[23] = (k + nsp) % ns + j + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[24] = (k + nsp) % ns + (j + xDim) % nsp + (i - 2 + xDim) % xDim + 1;
    n.neigh_id[25] = (k + nsp) % ns + (j + xDim) % nsp + i;
    n.neigh_id[26] = (k + nsp) % ns + (j + xDim) % nsp + i % xDim + 1;
    return n;
  }
};

/**
 * @brief Calculates the four corner sites of a marching square.
 * @param squareId Encodes the source site and square orientation.
 * @param neighbors Provides padded-grid neighbor indexes.
 * @return Corner site indexes in marching-square order.
 *
 * On-demand calculation keeps four site indexes out of every Face record.
 */
std::array<SiteId, 4> squareCorners(SiteId squareId, const NeighborAccessor& neighbors)
{
  const SiteId site = squareId / 3 + 1;
  const int ord = static_cast<int>(squareId % 3);
  const Neighbor nb = neighbors[site];
  switch(ord)
  {
  case 0: // top (same z)
    return {site, nb.neigh_id[1], nb.neigh_id[8], nb.neigh_id[7]};
  case 1: // back (same y)
    return {site, nb.neigh_id[1], nb.neigh_id[19], nb.neigh_id[18]};
  default: // left (same x)
    return {nb.neigh_id[7], site, nb.neigh_id[18], nb.neigh_id[25]};
  }
}

/**
 * @brief Initializes three empty marching squares per padded site.
 * @param sq Receives empty edge and flag fields.
 * @param ns Specifies padded site count.
 *
 * Candidate coordinates are calculated on demand. The node-type vector uses
 * value initialization, so neither data set needs a separate initialization pass.
 */
void initialize_squares(Face* sq, SiteId ns)
{
  for(SiteId sqId = 0; sqId < 3 * ns; sqId++)
  {
    for(int j = 0; j < 4; j++)
    {
      sq[sqId].edge_id[j] = k_UnusedNodeId;
    }
    sq[sqId].nEdge = 0;
    sq[sqId].FCnode = -1;
    sq[sqId].effect = 0;
  }
}

/**
 * @namespace M3CNodeType
 * @brief Defines node categories consumed by mesh-smoothing algorithms.
 *
 * These values match the legacy SurfaceMesh NodeType contract.
 */
namespace M3CNodeType
{
constexpr int8 k_Unused = 0;
constexpr int8 k_Default = 2;
constexpr int8 k_TriplePoint = 3;
constexpr int8 k_QuadPoint = 4;
constexpr int8 k_SurfaceDefault = 12;
constexpr int8 k_SurfaceTriplePoint = 13;
constexpr int8 k_SurfaceQuadPoint = 14;
} // namespace M3CNodeType

/**
 * @brief Classifies four corner labels into a marching-square case.
 * @param tns Provides four corner labels in square order.
 * @return Case index from 0 through 19.
 */
int get_square_index(const int tns[4])
{
  int aBit[6];
  aBit[0] = (tns[0] == tns[1]) ? 0 : 1;
  aBit[1] = (tns[1] == tns[2]) ? 0 : 1;
  aBit[2] = (tns[2] == tns[3]) ? 0 : 1;
  aBit[3] = (tns[3] == tns[0]) ? 0 : 1;
  aBit[4] = (tns[0] == tns[2]) ? 0 : 1;
  aBit[5] = (tns[1] == tns[3]) ? 0 : 1;

  int tempIndex = 8 * aBit[3] + 4 * aBit[2] + 2 * aBit[1] + 1 * aBit[0];
  if(tempIndex == 15)
  {
    int subIndex = 2 * aBit[4] + 1 * aBit[5];
    if(subIndex != 0)
    {
      tempIndex = tempIndex + subIndex + 1;
    }
  }
  return tempIndex;
}

/**
 * @brief Resolves the case-15 saddle from three-dimensional neighbors.
 * @param tnst Provides four corner site indexes.
 * @param p1 Provides padded Feature Id values.
 * @param n1 Calculates padded-grid neighbors.
 * @param sqid Is unused by the legacy-compatible calculation.
 * @return Zero or one to select the case-15 topology.
 *
 * The algorithm connects the corner with the fewest positive same-label neighbors.
 */
int treat_anomaly(const std::array<SiteId, 4>& tnst, const int32* p1, const NeighborAccessor& n1, SiteId /*sqid*/)
{
  int numNeigh[4] = {0, 0, 0, 0};

  for(int i = 0; i < 4; i++)
  {
    SiteId csite = tnst[i];
    int cspin = p1[csite];
    const Neighbor nb = n1[csite]; // cache: all 26 neighbors read below
    for(int j = 1; j <= num_neigh; j++)
    {
      SiteId nsite = nb.neigh_id[j];
      int nspin = p1[nsite];
      if(cspin == nspin && nspin > 0)
      {
        numNeigh[i] = numNeigh[i] + 1;
      }
    }
  }

  int min = 1000;
  int minid = -1;
  for(int ii = 0; ii < 4; ii++)
  {
    if(numNeigh[ii] < min)
    {
      min = numNeigh[ii];
      minid = ii;
    }
  }

  int tempFlag;
  if(minid == -1 || minid == 1 || minid == 3)
  {
    tempFlag = 0;
  }
  else
  {
    tempFlag = 1;
  }
  return tempFlag;
}

/**
 * @brief Maps edge-table slots to candidate-node indexes.
 * @param cst Specifies the square origin site.
 * @param ord Specifies the square orientation.
 * @param nidx Provides two edge-table node slots.
 * @param nid Receives two candidate-node indexes.
 * @param nsp1 Specifies padded sites per Z plane.
 * @param xDim1 Specifies padded X dimension.
 */
void get_nodes(SiteId cst, int ord, const int nidx[2], SiteId* nid, SiteId nsp1, int xDim1)
{
  for(int ii = 0; ii < 2; ii++)
  {
    int tempIndex = nidx[ii];
    if(ord == 0)
    {
      switch(tempIndex)
      {
      case 0:
        nid[ii] = 7 * (cst - 1);
        break;
      case 1:
        nid[ii] = 7 * cst + 1;
        break;
      case 2:
        nid[ii] = 7 * (cst + xDim1 - 1);
        break;
      case 3:
        nid[ii] = 7 * (cst - 1) + 1;
        break;
      case 4:
        nid[ii] = 7 * (cst - 1) + 3;
        break;
      }
    }
    else if(ord == 1)
    {
      switch(tempIndex)
      {
      case 0:
        nid[ii] = 7 * (cst - 1);
        break;
      case 1:
        nid[ii] = 7 * cst + 2;
        break;
      case 2:
        nid[ii] = 7 * (cst + nsp1 - 1);
        break;
      case 3:
        nid[ii] = 7 * (cst - 1) + 2;
        break;
      case 4:
        nid[ii] = 7 * (cst - 1) + 4;
        break;
      }
    }
    else
    {
      switch(tempIndex)
      {
      case 0:
        nid[ii] = 7 * (cst - 1) + 1;
        break;
      case 1:
        nid[ii] = 7 * (cst - 1) + 2;
        break;
      case 2:
        nid[ii] = 7 * (cst + nsp1 - 1) + 1;
        break;
      case 3:
        nid[ii] = 7 * (cst + xDim1 - 1) + 2;
        break;
      case 4:
        nid[ii] = 7 * (cst - 1) + 5;
        break;
      }
    }
  }
}

/**
 * @brief Maps square-corner slots to two edge-side labels.
 * @param p1 Provides padded Feature Id values.
 * @param cst Specifies the square origin site.
 * @param ord Specifies the square orientation.
 * @param pID Provides two square-corner slots.
 * @param pSpin Receives the two Feature Id values.
 * @param nsp1 Specifies padded sites per Z plane.
 * @param xDim1 Specifies padded X dimension.
 */
void get_spins(const int32* p1, SiteId cst, int ord, const int pID[2], int* pSpin, SiteId nsp1, int xDim1)
{
  for(int i = 0; i < 2; i++)
  {
    int pixTemp = pID[i];
    if(ord == 0)
    {
      switch(pixTemp)
      {
      case 0:
        pSpin[i] = p1[cst];
        break;
      case 1:
        pSpin[i] = p1[cst + 1];
        break;
      case 2:
        pSpin[i] = p1[cst + xDim1 + 1];
        break;
      case 3:
        pSpin[i] = p1[cst + xDim1];
        break;
      }
    }
    else if(ord == 1)
    {
      switch(pixTemp)
      {
      case 0:
        pSpin[i] = p1[cst];
        break;
      case 1:
        pSpin[i] = p1[cst + 1];
        break;
      case 2:
        pSpin[i] = p1[cst + nsp1 + 1];
        break;
      case 3:
        pSpin[i] = p1[cst + nsp1];
        break;
      }
    }
    else if(ord == 2)
    {
      switch(pixTemp)
      {
      case 0:
        pSpin[i] = p1[cst + xDim1];
        break;
      case 1:
        pSpin[i] = p1[cst];
        break;
      case 2:
        pSpin[i] = p1[cst + nsp1];
        break;
      case 3:
        pSpin[i] = p1[cst + nsp1 + xDim1];
        break;
      }
    }
  }
}

/**
 * @brief Counts face edges and marks effective squares.
 * @param sq Receives each square's effect flag.
 * @param p Provides padded Feature Id values.
 * @param n Calculates padded-grid neighbors.
 * @param ns Specifies padded site count.
 * @param shouldCancel Stops before later squares when true.
 * @return Count accumulated before completion or cancellation.
 *
 * The count permits one exact allocation before edge generation.
 */
int64 get_number_fEdges(Face* sq, const int32* p, const NeighborAccessor& n, SiteId ns, const std::atomic_bool& shouldCancel)
{
  int64 sumEdge = 0;
  for(SiteId k = 0; k < (3 * ns); k++)
  {
    if(shouldCancel)
    {
      return sumEdge;
    }
    const std::array<SiteId, 4> tnsite = squareCorners(k, n);
    int tnspin[4];
    int numGhostCorners = 0;
    for(int m = 0; m < 4; m++)
    {
      tnspin[m] = p[tnsite[m]];
      if(tnspin[m] < 0)
      {
        numGhostCorners++;
      }
    }
    if(numGhostCorners != 4)
    {
      sq[k].effect = 1; // mark as effective (can be marching-cubed)
    }

    if(numGhostCorners != 4)
    {
      int sqIndex = get_square_index(tnspin);
      if(sqIndex == 15)
      {
        sqIndex = sqIndex + treat_anomaly(tnsite, p, n, k);
      }

      int numCEdge = 0;
      if(sqIndex == 0)
      {
        numCEdge = 0;
      }
      else if(sqIndex == 19)
      {
        numCEdge = 4;
      }
      else if(sqIndex == 15 || sqIndex == 16 || sqIndex == 17 || sqIndex == 18)
      {
        numCEdge = 2;
      }
      else if(sqIndex == 7 || sqIndex == 11 || sqIndex == 13 || sqIndex == 14)
      {
        if(numGhostCorners == 3)
        {
          numCEdge = 2;
        }
        else if(numGhostCorners == 2)
        {
          numCEdge = 3;
        }
        else if(numGhostCorners == 1)
        {
          // A single negative corner is not a valid legacy square case.
          numCEdge = 0;
        }
        else
        {
          numCEdge = 3;
        }
      }
      else
      {
        numCEdge = 1;
      }
      sumEdge = sumEdge + numCEdge;
    }
  }
  return sumEdge;
}

/**
 * @brief Creates face edges and classifies their candidate nodes.
 * @param sq Receives edge indexes and face-center nodes.
 * @param p Provides padded Feature Id values.
 * @param n Calculates padded-grid neighbors.
 * @param nodeType Receives candidate-node categories.
 * @param e Receives face-edge records.
 * @param ns Specifies padded site count.
 * @param nsp Specifies padded sites per Z plane.
 * @param xDim Specifies padded X dimension.
 * @param shouldCancel Stops before later squares when true.
 */
void get_nodes_fEdges(Face* sq, const int32* p, const NeighborAccessor& n, int8* nodeType, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
{
  int64 eid = 0;
  for(SiteId k = 0; k < (3 * ns); k++)
  {
    if(shouldCancel)
    {
      return;
    }
    SiteId cubeOrigin = k / 3 + 1;
    int sqOrder = static_cast<int>(k % 3);

    const std::array<SiteId, 4> tnsite = squareCorners(k, n);
    int tnspin[4];
    int numGhostCorners = 0;
    for(int m = 0; m < 4; m++)
    {
      tnspin[m] = p[tnsite[m]];
      if(tnspin[m] < 0)
      {
        numGhostCorners++;
      }
    }

    int edgeCount = 0;
    if(numGhostCorners != 4)
    {
      int sqIndex = get_square_index(tnspin);
      if(sqIndex == 15)
      {
        sqIndex = sqIndex + treat_anomaly(tnsite, p, n, k);
      }
      if(sqIndex != 0)
      {
        for(int j = 0; j < 8; j = j + 2)
        {
          if(k_EdgeTable2d[sqIndex][j] != -1)
          {
            int nodeIndex[2] = {k_EdgeTable2d[sqIndex][j], k_EdgeTable2d[sqIndex][j + 1]};
            int pixIndex[2] = {k_NsTable2d[sqIndex][j], k_NsTable2d[sqIndex][j + 1]};
            SiteId nodeID[2];
            int pixSpin[2];
            get_nodes(cubeOrigin, sqOrder, nodeIndex, nodeID, nsp, xDim);
            get_spins(p, cubeOrigin, sqOrder, pixIndex, pixSpin, nsp, xDim);

            if(pixSpin[0] > 0 || pixSpin[1] > 0)
            {
              e[eid].node_id[0] = nodeID[0];
              e[eid].node_id[1] = nodeID[1];
              e[eid].nSpin[0] = pixSpin[0];
              e[eid].nSpin[1] = pixSpin[1];
              sq[k].edge_id[edgeCount] = static_cast<uint32>(eid);
              edgeCount++;
              eid++;
            }
            else
            {
              // Pure exterior edges do not create output mesh nodes.
              nodeType[nodeID[0]] = M3CNodeType::k_Unused;
              nodeType[nodeID[1]] = M3CNodeType::k_Unused;
            }

            // Face centers represent triple or quad points. Other slots represent
            // default interface nodes.
            for(int ii = 0; ii < 2; ii++)
            {
              if(nodeIndex[ii] == 4)
              {
                if(sqIndex == 7 || sqIndex == 11 || sqIndex == 13 || sqIndex == 14)
                {
                  SiteId tnode = nodeID[ii];
                  sq[k].FCnode = tnode;
                  nodeType[tnode] = M3CNodeType::k_TriplePoint;
                }
                else if(sqIndex == 19)
                {
                  SiteId tnode = nodeID[ii];
                  sq[k].FCnode = tnode;
                  nodeType[tnode] = M3CNodeType::k_QuadPoint;
                }
              }
              else
              {
                // Every interior edge endpoint is a real mesh node. Without this
                // promotion, compaction can remove a node that stored edges reference.
                SiteId tnode = nodeID[ii];
                nodeType[tnode] = M3CNodeType::k_Default;
              }
            }
          }
        }
      }
    }
    sq[k].nEdge = edgeCount;
  }
}

/**
 * @brief Counts triangles for a cube without face centers.
 * @param afe Provides cube face-edge indexes.
 * @param e1 Provides oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @return Triangle count after closed-loop fan triangulation.
 */
int get_number_case0_triangles(const SiteId* afe, Segment* e1, int nfedge)
{
  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);

        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag;
            int flip;
            if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
              flip = 0;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
              flip = 1;
            }
            else
            {
              nodeFlag = 0;
              flip = 0;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
              if(flip == 1)
              {
                e1[nedge].nSpin[0] = nspin2;
                e1[nedge].nSpin[1] = nspin1;
                e1[nedge].node_id[0] = nnode2;
                e1[nedge].node_id[1] = nnode1;
              }
            }
          }
        }

        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  int numTri = 0;
  for(int jj = 1; jj < loopID; jj++)
  {
    int numN = count[jj];
    if(numN == 3)
    {
      numTri = numTri + 1;
    }
    else if(numN > 3)
    {
      numTri = numTri + (numN - 2);
    }
  }
  return numTri;
}

/**
 * @brief Counts triangles for a cube with two face centers.
 * @param afe Provides cube face-edge indexes.
 * @param e1 Provides mutable oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @param afc Provides face-center node indexes.
 * @param nfctr Is fixed at two and is unused.
 * @return Triangle count after open- and closed-loop triangulation.
 *
 * Valid label data extends each chase loop by one edge. Loop guards bound
 * malformed or non-manifold input instead of overrunning a buffer.
 */
int get_number_case2_triangles(const SiteId* afe, Segment* e1, int nfedge, const SiteId* afc, int /*nfctr*/)
{
  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);

        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag = 0;
            if((cnode1 == nnode1) && (cnode2 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode1 == nnode2) && (cnode2 != nnode1))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
            }
          }
        }

        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  int numTri = 0;
  SiteId start = afc[0];
  int to = 0;
  int from = 0;

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<usize>(numN) + 2, 0);

    for(int i1 = from; i1 < to; i1++)
    {
      SiteId cedge = burnt_list[i1];
      SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
      SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
      if(start == cnode1)
      {
        openL = 1;
        startEdge = cedge;
        flip = 0;
      }
      else if(start == cnode2)
      {
        openL = 1;
        startEdge = cedge;
        flip = 1;
      }
    }

    if(openL == 1)
    {
      if(flip == 1)
      {
        SiteId tnode = static_cast<SiteId>(e1[startEdge].node_id[0]);
        int tspin = e1[startEdge].nSpin[0];
        e1[startEdge].node_id[0] = e1[startEdge].node_id[1];
        e1[startEdge].node_id[1] = tnode;
        e1[startEdge].nSpin[0] = e1[startEdge].nSpin[1];
        e1[startEdge].nSpin[1] = tspin;
      }

      burnt_loop[0] = startEdge;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge].node_id[1]);
      SiteId chaser = startEdge;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if((numN + 1) == 3)
      {
        numTri = numTri + 1;
      }
      else if((numN + 1) > 3)
      {
        numTri = numTri + ((numN + 1) - 2);
      }
    }
    else
    {
      SiteId startEdge2 = burnt_list[from];
      burnt_loop[0] = startEdge2;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge2].node_id[1]);
      SiteId chaser = startEdge2;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if(numN == 3)
      {
        numTri = numTri + 1;
      }
      else if(numN > 3)
      {
        numTri = numTri + (numN - 2);
      }
    }
  }
  return numTri;
}

/**
 * @brief Counts triangles for a cube with three or more face centers.
 * @param afe Provides cube face-edge indexes.
 * @param e1 Provides mutable oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @param afc Provides face-center node indexes.
 * @param nfctr Specifies face-center count.
 * @return Triangle count after body-center and closed-loop triangulation.
 */
int get_number_caseM_triangles(const SiteId* afe, Segment* e1, int nfedge, const SiteId* afc, int nfctr)
{
  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);

        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag = 0;
            if((cnode1 == nnode1) && (cnode2 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode1 == nnode2) && (cnode2 != nnode1))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
            }
          }
        }

        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  int numTri = 0;
  int to = 0;
  int from = 0;

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<usize>(numN) + 2, 0);

    for(int i1 = from; i1 < to; i1++)
    {
      SiteId cedge = burnt_list[i1];
      SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
      SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
      for(int n1 = 0; n1 < nfctr; n1++)
      {
        SiteId start = afc[n1];
        if(start == cnode1)
        {
          openL = 1;
          startEdge = cedge;
          flip = 0;
        }
        else if(start == cnode2)
        {
          openL = 1;
          startEdge = cedge;
          flip = 1;
        }
      }
    }

    if(openL == 1)
    {
      if(flip == 1)
      {
        SiteId tnode = static_cast<SiteId>(e1[startEdge].node_id[0]);
        int tspin = e1[startEdge].nSpin[0];
        e1[startEdge].node_id[0] = e1[startEdge].node_id[1];
        e1[startEdge].node_id[1] = tnode;
        e1[startEdge].nSpin[0] = e1[startEdge].nSpin[1];
        e1[startEdge].nSpin[1] = tspin;
      }

      burnt_loop[0] = startEdge;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge].node_id[1]);
      SiteId chaser = startEdge;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if((numN + 2) == 3)
      {
        numTri = numTri + 1;
      }
      else if((numN + 2) > 3)
      {
        numTri = numTri + ((numN + 2) - 2);
      }
    }
    else
    {
      SiteId startEdge2 = burnt_list[from];
      burnt_loop[0] = startEdge2;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge2].node_id[1]);
      SiteId chaser = startEdge2;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if(numN == 3)
      {
        numTri = numTri + 1;
      }
      else if(numN > 3)
      {
        numTri = numTri + (numN - 2);
      }
    }
  }
  return numTri;
}

/**
 * @brief Counts all triangles and classifies body-center nodes.
 * @param p Provides padded Feature Id values.
 * @param sq Provides marching-square records.
 * @param neighbors Calculates padded-grid neighbors.
 * @param nodeType Receives body-center node categories.
 * @param e Provides mutable oriented face-edge records.
 * @param ns Specifies padded site count.
 * @param nsp Specifies padded sites per Z plane.
 * @param xDim Specifies padded X dimension.
 * @param shouldCancel Stops before later cubes when true.
 * @return Triangle count accumulated before completion or cancellation.
 */
int64 get_number_triangles(const int32* p, Face* sq, const NeighborAccessor& neighbors, int8* nodeType, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
{
  int64 nTri0 = 0;
  int64 nTri2 = 0;
  int64 nTriM = 0;

  for(SiteId i = 1; i <= (ns - nsp); i++)
  {
    if(shouldCancel)
    {
      return 0;
    }
    int cubeFlag = 0;
    SiteId sqID[6];
    sqID[0] = 3 * (i - 1);
    sqID[1] = 3 * (i - 1) + 1;
    sqID[2] = 3 * (i - 1) + 2;
    sqID[3] = 3 * i + 2;
    sqID[4] = 3 * (i + xDim - 1) + 1;
    sqID[5] = 3 * (i + nsp - 1);
    SiteId BCnode = 7 * (i - 1) + 6;
    int nFC = 0;
    int nFE = 0;
    int eff = 0;
    SiteId arrayFC[6];
    for(int ii = 0; ii < 6; ii++)
    {
      arrayFC[ii] = -1;
    }
    int fcid = 0;
    for(int ii = 0; ii < 6; ii++)
    {
      int tsq = sqID[ii];
      SiteId tFCnode = sq[tsq].FCnode;
      if(tFCnode != -1)
      {
        arrayFC[fcid] = tFCnode;
        fcid++;
      }
      nFE = nFE + sq[tsq].nEdge;
      eff = eff + sq[tsq].effect;
    }
    nFC = fcid;
    if(eff > 0)
    {
      cubeFlag = 1;
    }

    if(nFC >= 3)
    {
      const std::array<SiteId, 4> corners1 = squareCorners(sqID[0], neighbors);
      const std::array<SiteId, 4> corners2 = squareCorners(sqID[5], neighbors);
      int arraySpin[8];
      for(int j = 0; j < 4; j++)
      {
        arraySpin[j] = p[corners1[j]];
        arraySpin[j + 4] = p[corners2[j]];
      }
      int nds = 0;
      int nburnt = 0;
      for(int k = 0; k < 8; k++)
      {
        int cspin = arraySpin[k];
        if(cspin != -1)
        {
          nds++;
          arraySpin[k] = -1;
          nburnt++;
          for(int kk = 0; kk < 8; kk++)
          {
            if(cspin == arraySpin[kk])
            {
              arraySpin[kk] = -1;
              nburnt++;
            }
          }
        }
      }
      (void)nburnt;
      // Five or more labels can meet at a body center. NodeType supports only
      // the "four or more" category used by downstream mesh consumers.
      nodeType[BCnode] = static_cast<int8>(std::min(nds, static_cast<int>(M3CNodeType::k_QuadPoint)));
    }

    if(cubeFlag == 1 && nFE > 2)
    {
      std::vector<SiteId> arrayFE(nFE);
      int tindex = 0;
      for(int i1 = 0; i1 < 6; i1++)
      {
        int tsq = sqID[i1];
        int tnfe = sq[tsq].nEdge;
        for(int i2 = 0; i2 < tnfe; i2++)
        {
          arrayFE[tindex] = sq[tsq].edge_id[i2];
          tindex++;
        }
      }

      // Square cases determine face-center count. A cube can have zero or two
      // through six centers. One crossing cannot terminate inside one cube.
      if(nFC == 0)
      {
        nTri0 = nTri0 + get_number_case0_triangles(arrayFE.data(), e, nFE);
      }
      else if(nFC == 2)
      {
        nTri2 = nTri2 + get_number_case2_triangles(arrayFE.data(), e, nFE, arrayFC, nFC);
      }
      else if(nFC > 2 && nFC <= 6)
      {
        nTriM = nTriM + get_number_caseM_triangles(arrayFE.data(), e, nFE, arrayFC, nFC);
      }
    }
  }
  return nTri0 + nTri2 + nTriM;
}

/**
 * @brief Generates triangles for a cube without face centers.
 * @param t1 Receives triangle records.
 * @param mCubeID Receives the source cube for each triangle.
 * @param afe Provides cube face-edge indexes.
 * @param v1 Is retained by the legacy call shape and is unused.
 * @param e1 Provides mutable oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @param tin Specifies the first output triangle index.
 * @param tout Receives the next unused output triangle index.
 * @param tcrd1 Is retained by the legacy call shape and is unused.
 * @param tcrd2 Is retained by the legacy call shape and is unused.
 * @param mcid Specifies the source cube index.
 */
void get_case0_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, int64 tin, int64* tout, const double tcrd1[3], const double tcrd2[3],
                         SiteId mcid)
{

  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);
        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag;
            int flip;
            if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
              flip = 0;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
              flip = 1;
            }
            else
            {
              nodeFlag = 0;
              flip = 0;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
              if(flip == 1)
              {
                e1[nedge].nSpin[0] = nspin2;
                e1[nedge].nSpin[1] = nspin1;
                e1[nedge].node_id[0] = nnode2;
                e1[nedge].node_id[1] = nnode1;
              }
            }
          }
        }
        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  int sumN = 0;
  int64 ctid = tin;

  for(int jj = 1; jj < loopID; jj++)
  {
    int numN = count[jj];
    sumN = sumN + numN;
    int from = sumN - numN;
    std::vector<SiteId> loop(numN);
    for(int mm = 0; mm < numN; mm++)
    {
      loop[mm] = burnt_list[from + mm];
    }

    if(numN == 3)
    {
      SiteId te0 = loop[0], te1 = loop[1], te2 = loop[2];
      SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
      SiteId tv1 = static_cast<SiteId>(e1[te1].node_id[0]);
      SiteId tv2 = static_cast<SiteId>(e1[te2].node_id[0]);
      t1[ctid].node_id[0] = tv0;
      t1[ctid].node_id[1] = tv1;
      t1[ctid].node_id[2] = tv2;
      t1[ctid].nSpin[0] = e1[te0].nSpin[0];
      t1[ctid].nSpin[1] = e1[te0].nSpin[1];
      mCubeID[ctid] = mcid;
      ctid++;
    }
    else if(numN > 3)
    {
      int numT = numN - 2;
      int cnumT = 0;
      int front = 0;
      int back = numN - 1;

      SiteId te0 = loop[front];
      SiteId te1 = loop[back];
      SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
      SiteId tv1 = static_cast<SiteId>(e1[te0].node_id[1]);
      SiteId tv2 = static_cast<SiteId>(e1[te1].node_id[0]);
      t1[ctid].node_id[0] = tv0;
      t1[ctid].node_id[1] = tv1;
      t1[ctid].node_id[2] = tv2;
      t1[ctid].nSpin[0] = e1[te0].nSpin[0];
      t1[ctid].nSpin[1] = e1[te0].nSpin[1];
      mCubeID[ctid] = mcid;
      int new_node0 = tv2;
      cnumT++;
      ctid++;

      do
      {
        if((cnumT % 2) != 0)
        {
          front = front + 1;
          SiteId ce = loop[front];
          tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
          tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
          tv2 = new_node0;
          t1[ctid].node_id[0] = tv0;
          t1[ctid].node_id[1] = tv1;
          t1[ctid].node_id[2] = tv2;
          t1[ctid].nSpin[0] = e1[ce].nSpin[0];
          t1[ctid].nSpin[1] = e1[ce].nSpin[1];
          mCubeID[ctid] = mcid;
          new_node0 = tv1;
          cnumT++;
          ctid++;
        }
        else
        {
          back = back - 1;
          SiteId ce = loop[back];
          tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
          tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
          tv2 = new_node0;
          t1[ctid].node_id[0] = tv0;
          t1[ctid].node_id[1] = tv1;
          t1[ctid].node_id[2] = tv2;
          t1[ctid].nSpin[0] = e1[ce].nSpin[0];
          t1[ctid].nSpin[1] = e1[ce].nSpin[1];
          mCubeID[ctid] = mcid;
          new_node0 = tv0;
          cnumT++;
          ctid++;
        }
      } while(cnumT < numT);
    }
  }
  *tout = ctid;
}

/**
 * @brief Generates triangles for a cube with two face centers.
 * @param t1 Receives triangle records.
 * @param mCubeID Receives the source cube for each triangle.
 * @param afe Provides cube face-edge indexes.
 * @param v1 Is retained by the legacy call shape and is unused.
 * @param e1 Provides mutable oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @param afc Provides face-center node indexes.
 * @param nfctr Is fixed at two and is unused.
 * @param tin Specifies the first output triangle index.
 * @param tout Receives the next unused output triangle index.
 * @param tcrd1 Is retained by the legacy call shape and is unused.
 * @param tcrd2 Is retained by the legacy call shape and is unused.
 * @param mcid Specifies the source cube index.
 */
void get_case2_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, const SiteId* afc, int /*nfctr*/, int64 tin, int64* tout,
                         const double tcrd1[3], const double tcrd2[3], SiteId mcid)
{

  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);
        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag = 0;
            if((cnode1 == nnode1) && (cnode2 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode1 == nnode2) && (cnode2 != nnode1))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
            }
          }
        }
        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  SiteId start = afc[0];
  int to = 0;
  int from = 0;
  int64 ctid = tin;

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<usize>(numN) + 2, 0);

    for(int i1 = from; i1 < to; i1++)
    {
      SiteId cedge = burnt_list[i1];
      SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
      SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
      if(start == cnode1)
      {
        openL = 1;
        startEdge = cedge;
        flip = 0;
      }
      else if(start == cnode2)
      {
        openL = 1;
        startEdge = cedge;
        flip = 1;
      }
    }

    if(openL == 1)
    {
      if(flip == 1)
      {
        SiteId tnode = static_cast<SiteId>(e1[startEdge].node_id[0]);
        int tspin = e1[startEdge].nSpin[0];
        e1[startEdge].node_id[0] = e1[startEdge].node_id[1];
        e1[startEdge].node_id[1] = tnode;
        e1[startEdge].nSpin[0] = e1[startEdge].nSpin[1];
        e1[startEdge].nSpin[1] = tspin;
      }
      burnt_loop[0] = startEdge;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge].node_id[1]);
      SiteId chaser = startEdge;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if(numN == 2)
      {
        SiteId te0 = burnt_loop[0], te1 = burnt_loop[1];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te1].node_id[0]);
        SiteId tv2 = static_cast<SiteId>(e1[te1].node_id[1]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        ctid++;
      }
      else if(numN > 2)
      {
        int numT = numN - 1;
        int cnumT = 0;
        int front = 0;
        int back = numN;
        SiteId te0 = burnt_loop[front];
        SiteId te1 = burnt_loop[back - 1];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te0].node_id[1]);
        SiteId tv2 = static_cast<SiteId>(e1[te1].node_id[1]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        int new_node0 = tv2;
        cnumT++;
        ctid++;
        do
        {
          if((cnumT % 2) != 0)
          {
            front = front + 1;
            SiteId ce = burnt_loop[front];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv1;
            cnumT++;
            ctid++;
          }
          else
          {
            back = back - 1;
            SiteId ce = burnt_loop[back];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv0;
            cnumT++;
            ctid++;
          }
        } while(cnumT < numT);
      }
    }
    else
    {
      SiteId startEdge2 = burnt_list[from];
      burnt_loop[0] = startEdge2;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge2].node_id[1]);
      SiteId chaser = startEdge2;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if(numN == 3)
      {
        SiteId te0 = burnt_loop[0], te1 = burnt_loop[1], te2 = burnt_loop[2];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te1].node_id[0]);
        SiteId tv2 = static_cast<SiteId>(e1[te2].node_id[0]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        ctid++;
      }
      else if(numN > 3)
      {
        int numT = numN - 2;
        int cnumT = 0;
        int front = 0;
        int back = numN - 1;
        SiteId te0 = burnt_loop[front];
        SiteId te1 = burnt_loop[back];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te0].node_id[1]);
        SiteId tv2 = static_cast<SiteId>(e1[te1].node_id[0]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        int new_node0 = tv2;
        cnumT++;
        ctid++;
        do
        {
          if((cnumT % 2) != 0)
          {
            front = front + 1;
            SiteId ce = burnt_loop[front];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv1;
            cnumT++;
            ctid++;
          }
          else
          {
            back = back - 1;
            SiteId ce = burnt_loop[back];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv0;
            cnumT++;
            ctid++;
          }
        } while(cnumT < numT);
      }
    }
  }
  *tout = ctid;
}

/**
 * @brief Generates triangles for a cube with three or more face centers.
 * @param t1 Receives triangle records.
 * @param mCubeID Receives the source cube for each triangle.
 * @param afe Provides cube face-edge indexes.
 * @param v1 Is retained by the legacy call shape and is unused.
 * @param e1 Provides mutable oriented face-edge records.
 * @param nfedge Specifies cube face-edge count.
 * @param afc Provides face-center node indexes.
 * @param nfctr Specifies face-center count.
 * @param tin Specifies the first output triangle index.
 * @param tout Receives the next unused output triangle index.
 * @param ccn Specifies the body-center candidate node.
 * @param tcrd1 Is retained by the legacy call shape and is unused.
 * @param tcrd2 Is retained by the legacy call shape and is unused.
 * @param mcid Specifies the source cube index.
 *
 * Open loops use a fan from the body-center node.
 */
void get_caseM_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, const SiteId* afc, int nfctr, int64 tin, int64* tout, SiteId ccn,
                         const double tcrd1[3], const double tcrd2[3], SiteId mcid)
{

  std::vector<int> burnt(nfedge, 0);
  std::vector<SiteId> burnt_list(nfedge, -1);

  int loopID = 1;
  int tail = 0;
  int head = 0;

  for(int i = 0; i < nfedge; i++)
  {
    SiteId cedge = afe[i];
    if(burnt[i] == 0)
    {
      burnt[i] = loopID;
      burnt_list[tail] = cedge;
      int coin;
      do
      {
        SiteId chaser = burnt_list[tail];
        int cspin1 = e1[chaser].nSpin[0];
        int cspin2 = e1[chaser].nSpin[1];
        SiteId cnode1 = static_cast<SiteId>(e1[chaser].node_id[0]);
        SiteId cnode2 = static_cast<SiteId>(e1[chaser].node_id[1]);
        for(int j = 0; j < nfedge; j++)
        {
          SiteId nedge = afe[j];
          if(burnt[j] == 0)
          {
            int nspin1 = e1[nedge].nSpin[0];
            int nspin2 = e1[nedge].nSpin[1];
            SiteId nnode1 = static_cast<SiteId>(e1[nedge].node_id[0]);
            SiteId nnode2 = static_cast<SiteId>(e1[nedge].node_id[1]);
            int spinFlag = (((cspin1 == nspin1) && (cspin2 == nspin2)) || ((cspin1 == nspin2) && (cspin2 == nspin1))) ? 1 : 0;
            int nodeFlag = 0;
            if((cnode1 == nnode1) && (cnode2 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode1 == nnode2) && (cnode2 != nnode1))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode1) && (cnode1 != nnode2))
            {
              nodeFlag = 1;
            }
            else if((cnode2 == nnode2) && (cnode1 != nnode1))
            {
              nodeFlag = 1;
            }
            if(spinFlag == 1 && nodeFlag == 1)
            {
              head = head + 1;
              burnt_list[head] = nedge;
              burnt[j] = loopID;
            }
          }
        }
        if(tail == head)
        {
          coin = 0;
          tail = tail + 1;
          head = tail;
          loopID++;
        }
        else
        {
          tail = tail + 1;
          coin = 1;
        }
      } while(coin);
    }
  }

  std::vector<int> count(loopID, 0);
  for(int k = 1; k < loopID; k++)
  {
    for(int kk = 0; kk < nfedge; kk++)
    {
      if(k == burnt[kk])
      {
        count[k] = count[k] + 1;
      }
    }
  }

  int to = 0;
  int from = 0;
  int64 ctid = tin;

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<usize>(numN) + 2, 0);

    for(int i1 = from; i1 < to; i1++)
    {
      SiteId cedge = burnt_list[i1];
      SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
      SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
      for(int n1 = 0; n1 < nfctr; n1++)
      {
        SiteId start = afc[n1];
        if(start == cnode1)
        {
          openL = 1;
          startEdge = cedge;
          flip = 0;
        }
        else if(start == cnode2)
        {
          openL = 1;
          startEdge = cedge;
          flip = 1;
        }
      }
    }

    if(openL == 1)
    {
      if(flip == 1)
      {
        SiteId tnode = static_cast<SiteId>(e1[startEdge].node_id[0]);
        int tspin = e1[startEdge].nSpin[0];
        e1[startEdge].node_id[0] = e1[startEdge].node_id[1];
        e1[startEdge].node_id[1] = tnode;
        e1[startEdge].nSpin[0] = e1[startEdge].nSpin[1];
        e1[startEdge].nSpin[1] = tspin;
      }
      burnt_loop[0] = startEdge;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge].node_id[1]);
      SiteId chaser = startEdge;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      // Open loops use a fan from the body-center node.
      for(int iii = 0; iii < numN; iii++)
      {
        SiteId ce = burnt_loop[iii];
        SiteId tn0 = static_cast<SiteId>(e1[ce].node_id[0]);
        SiteId tn1 = static_cast<SiteId>(e1[ce].node_id[1]);
        int ts0 = e1[ce].nSpin[0];
        int ts1 = e1[ce].nSpin[1];
        t1[ctid].node_id[0] = ccn;
        t1[ctid].node_id[1] = tn0;
        t1[ctid].node_id[2] = tn1;
        t1[ctid].nSpin[0] = ts0;
        t1[ctid].nSpin[1] = ts1;
        mCubeID[ctid] = mcid;
        ctid++;
      }
    }
    else
    {
      SiteId startEdge2 = burnt_list[from];
      burnt_loop[0] = startEdge2;
      int index = 1;
      SiteId endNode = static_cast<SiteId>(e1[startEdge2].node_id[1]);
      SiteId chaser = startEdge2;
      do
      {
        const int passStart = index; // chase-loop guard: detect a pass that fails to extend the chain
        for(int n = from; n < to; n++)
        {
          SiteId cedge = burnt_list[n];
          SiteId cnode1 = static_cast<SiteId>(e1[cedge].node_id[0]);
          SiteId cnode2 = static_cast<SiteId>(e1[cedge].node_id[1]);
          if((cedge != chaser) && (endNode == cnode1))
          {
            burnt_loop[index] = cedge;
            index++;
          }
          else if((cedge != chaser) && (endNode == cnode2))
          {
            burnt_loop[index] = cedge;
            index++;
            SiteId tnode = static_cast<SiteId>(e1[cedge].node_id[0]);
            int tspin = e1[cedge].nSpin[0];
            e1[cedge].node_id[0] = e1[cedge].node_id[1];
            e1[cedge].node_id[1] = tnode;
            e1[cedge].nSpin[0] = e1[cedge].nSpin[1];
            e1[cedge].nSpin[1] = tspin;
          }
          if(index >= numN)
          {
            break; // chain complete; also caps degenerate multi-match passes so burnt_loop cannot overrun
          }
        }
        if(index == passStart)
        {
          break; // degenerate input: the pass matched no edge, so the chain can never close
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      if(numN == 3)
      {
        SiteId te0 = burnt_loop[0], te1 = burnt_loop[1], te2 = burnt_loop[2];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te1].node_id[0]);
        SiteId tv2 = static_cast<SiteId>(e1[te2].node_id[0]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        ctid++;
      }
      else if(numN > 3)
      {
        int numT = numN - 2;
        int cnumT = 0;
        int front = 0;
        int back = numN - 1;
        SiteId te0 = burnt_loop[front];
        SiteId te1 = burnt_loop[back];
        SiteId tv0 = static_cast<SiteId>(e1[te0].node_id[0]);
        SiteId tv1 = static_cast<SiteId>(e1[te0].node_id[1]);
        SiteId tv2 = static_cast<SiteId>(e1[te1].node_id[0]);
        t1[ctid].node_id[0] = tv0;
        t1[ctid].node_id[1] = tv1;
        t1[ctid].node_id[2] = tv2;
        t1[ctid].nSpin[0] = e1[te0].nSpin[0];
        t1[ctid].nSpin[1] = e1[te0].nSpin[1];
        mCubeID[ctid] = mcid;
        int new_node0 = tv2;
        cnumT++;
        ctid++;
        do
        {
          if((cnumT % 2) != 0)
          {
            front = front + 1;
            SiteId ce = burnt_loop[front];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv1;
            cnumT++;
            ctid++;
          }
          else
          {
            back = back - 1;
            SiteId ce = burnt_loop[back];
            tv0 = static_cast<SiteId>(e1[ce].node_id[0]);
            tv1 = static_cast<SiteId>(e1[ce].node_id[1]);
            tv2 = new_node0;
            t1[ctid].node_id[0] = tv0;
            t1[ctid].node_id[1] = tv1;
            t1[ctid].node_id[2] = tv2;
            t1[ctid].nSpin[0] = e1[ce].nSpin[0];
            t1[ctid].nSpin[1] = e1[ce].nSpin[1];
            mCubeID[ctid] = mcid;
            new_node0 = tv0;
            cnumT++;
            ctid++;
          }
        } while(cnumT < numT);
      }
    }
  }
  *tout = ctid;
}

/**
 * @brief Fills pre-sized triangle arrays in cube order.
 * @param p Calculates padded-site coordinates.
 * @param t Receives triangle records.
 * @param mCubeID Receives the source cube for each triangle.
 * @param sq Provides marching-square records.
 * @param v Calculates candidate-node coordinates.
 * @param e Provides mutable oriented face-edge records.
 * @param ns Specifies padded site count.
 * @param nsp Specifies padded sites per Z plane.
 * @param xDim Specifies padded X dimension.
 * @param shouldCancel Stops before later cubes when true.
 */
void get_triangles(const SiteCoords& p, Triangle* t, SiteId* mCubeID, Face* sq, const NodeCoords& v, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
{
  int64 tidIn = 0;
  int64 tidOut = 0;

  for(SiteId i = 1; i <= (ns - nsp); i++)
  {
    if(shouldCancel)
    {
      return;
    }
    int cubeFlag = 0;
    SiteId sqID[6];
    sqID[0] = 3 * (i - 1);
    sqID[1] = 3 * (i - 1) + 1;
    sqID[2] = 3 * (i - 1) + 2;
    sqID[3] = 3 * i + 2;
    sqID[4] = 3 * (i + xDim - 1) + 1;
    sqID[5] = 3 * (i + nsp - 1);
    int nFC = 0;
    int nFE = 0;
    int eff = 0;
    SiteId bodyCtr = 7 * (i - 1) + 6;
    SiteId arrayFC[6];
    for(int ii = 0; ii < 6; ii++)
    {
      arrayFC[ii] = -1;
    }
    int fcid = 0;
    for(int ii = 0; ii < 6; ii++)
    {
      int tsq = sqID[ii];
      SiteId tFCnode = sq[tsq].FCnode;
      if(tFCnode != -1)
      {
        arrayFC[fcid] = tFCnode;
        fcid++;
      }
      nFE = nFE + sq[tsq].nEdge;
      eff = eff + sq[tsq].effect;
    }
    nFC = fcid;
    if(eff > 0)
    {
      cubeFlag = 1;
    }

    if(cubeFlag == 1 && nFE > 2)
    {
      double coord1[3], coord2[3];
      for(int k = 0; k < 3; k++)
      {
        coord1[k] = p[i].coord[k];
        coord2[k] = p[i + 1 + xDim + nsp].coord[k];
      }
      std::vector<SiteId> arrayFE(nFE);
      int tindex = 0;
      for(int i1 = 0; i1 < 6; i1++)
      {
        int tsq = sqID[i1];
        int tnfe = sq[tsq].nEdge;
        for(int i2 = 0; i2 < tnfe; i2++)
        {
          arrayFE[tindex] = sq[tsq].edge_id[i2];
          tindex++;
        }
      }

      if(nFC == 0)
      {
        get_case0_triangles(t, mCubeID, arrayFE.data(), v, e, nFE, tidIn, &tidOut, coord1, coord2, i);
        tidIn = tidOut;
      }
      else if(nFC == 2)
      {
        get_case2_triangles(t, mCubeID, arrayFE.data(), v, e, nFE, arrayFC, nFC, tidIn, &tidOut, coord1, coord2, i);
        tidIn = tidOut;
      }
      else if(nFC > 2 && nFC <= 6)
      {
        get_caseM_triangles(t, mCubeID, arrayFE.data(), v, e, nFE, arrayFC, nFC, tidIn, &tidOut, bodyCtr, coord1, coord2, i);
        tidIn = tidOut;
      }
    }
  }
}

/**
 * @brief Converts a padded site to an original cell index.
 * @param site Specifies the one-based padded site.
 * @param fileDim Specifies padded grid dimensions.
 * @param dims Specifies original grid dimensions.
 * @return Original zero-based cell index, or SIZE_MAX for a ghost site.
 */
usize paddedSiteToOriginalCell(int64 site, const usize fileDim[3], const usize dims[3])
{
  const usize linear = static_cast<usize>(site - 1);
  const usize px = linear % fileDim[0];
  const usize py = (linear / fileDim[0]) % fileDim[1];
  const usize pz = linear / (fileDim[0] * fileDim[1]);
  if(px >= 1 && px <= dims[0] && py >= 1 && py <= dims[1] && pz >= 1 && pz <= dims[2])
  {
    return (pz - 1) * dims[0] * dims[1] + (py - 1) * dims[0] + (px - 1);
  }
  return std::numeric_limits<usize>::max();
}

/**
 * @brief Finds a non-ghost source cell for one working label.
 * @param workLabel Specifies the renumbered Feature Id.
 * @param cubeSite Specifies the cube origin site.
 * @param n Provides cube neighbors.
 * @param point Provides padded Feature Id values.
 * @param fileDim Specifies padded grid dimensions.
 * @param dims Specifies original grid dimensions.
 * @return Original cell index, or SIZE_MAX when the label is exterior.
 */
usize findSourceCell(int workLabel, int64 cubeSite, const NeighborAccessor& n, const int32* point, const usize fileDim[3], const usize dims[3])
{
  const Neighbor nb = n[cubeSite]; // cache: 7 neighbors of the cube site read below
  const int64 cornerSites[8] = {cubeSite, nb.neigh_id[1], nb.neigh_id[7], nb.neigh_id[8], nb.neigh_id[18], nb.neigh_id[19], nb.neigh_id[25], nb.neigh_id[26]};
  for(int64 site : cornerSites)
  {
    if(point[site] == workLabel)
    {
      const usize original = paddedSiteToOriginalCell(site, fileDim, dims);
      if(original != std::numeric_limits<usize>::max())
      {
        return original;
      }
    }
  }
  return std::numeric_limits<usize>::max();
}

/**
 * @brief Finalizes mesh topology, transfers arrays, and repairs winding.
 * @param dataStructure Provides input and output objects.
 * @param inputValues Specifies output paths and options.
 * @param messageHandler Receives progress messages.
 * @param shouldCancel Stops later finalization stages when true.
 * @param triangles Provides generated triangle records.
 * @param mCubeID Provides triangle cube indexes.
 * @param fedges Provides face-edge scratch records.
 * @param nodeType Provides candidate node types.
 * @param point Provides padded Feature Id values.
 * @param nodeCoords Calculates node coordinates.
 * @param neighbors Provides padded-grid neighbors.
 * @param numSites Specifies padded-grid site count.
 * @param fileDim Specifies padded grid dimensions.
 * @param dims Specifies original grid dimensions.
 * @param maxGrainId Specifies the reserved zero-label value.
 * @return Error during output or transfer, or success after cancellation.
 */
Result<> finalizeMesh(DataStructure& dataStructure, const M3CSurfaceMeshingInputValues* inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                      std::vector<Triangle>& triangles, std::vector<SiteId>& mCubeID, std::vector<Segment>& fedges, std::vector<int8>& nodeType, std::vector<int32>& point,
                      const NodeCoords& nodeCoords, const NeighborAccessor& neighbors, SiteId numSites, const usize* fileDim, const usize* dims, int maxGrainId)
{
  const int64 nTriangle = static_cast<int64>(triangles.size());

  // Faces suppressed by the prune below. Stays 0 when the mode is Off, or when the mode is on but
  // nothing matched. Read after the winding-repair pass to decide which (if either) of the two
  // Bounding Box Skin warnings to emit.
  int64 numFacesPruned = 0;

  // Bounding Box Skin option, 'Background-Backed Walls Only' mode: drop faces whose output Face Labels would be {-1, 0}. In the
  // internal representation that is one negative ghost label paired with maxGrainId, which
  // is the renumbered zero-feature (see toFaceLabel below). Pruning the scratch vectors here
  // means the output TriangleGeom is sized from the surviving count and never over-allocated.
  if(inputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    messageHandler("Omitting bounding box skin faces...");

    // True when this triangle is a bounding-box wall face backed by background (i.e. its output
    // Face Labels would be {-1, 0}). M3C's single sequential pass over `triangles` (below) is the
    // only place this predicate is evaluated, so -- unlike QuickSurfaceMesh's SkipWallFace and
    // SurfaceNets' SkipPaddingQuad -- there is no second pass it must stay in agreement with.
    const auto SkipBackgroundSkinFace = [maxGrainId](const Triangle& triangle) -> bool {
      const int spinA = triangle.nSpin[0];
      const int spinB = triangle.nSpin[1];
      return (spinA < 0 && spinB == maxGrainId) || (spinB < 0 && spinA == maxGrainId);
    };

    // Count how many triangles will be dropped before allocating droppedNodeIds: pushing onto an
    // unreserved vector here causes dozens of reallocations (and a transient ~1.5x peak) on a
    // representative dataset. This adds a second pass over `triangles`, but it only evaluates the
    // same boolean predicate the compaction loop below already does -- no allocation -- so it is
    // negligible next to the reallocations it avoids.
    int64 numToDrop = 0;
    for(int64 i = 0; i < nTriangle; i++)
    {
      if(SkipBackgroundSkinFace(triangles[static_cast<usize>(i)]))
      {
        numToDrop++;
      }
    }

    int64 survivingCount = 0;
    // node_id values touched by a DROPPED triangle, recorded before the in-place compaction below
    // overwrites them. Used to narrow the nodeType clear (see below) to exactly the nodes the prune
    // itself orphaned, at a cost of O(3 * droppedCount) instead of a second full 7*numSites mask.
    std::vector<SiteId> droppedNodeIds;
    droppedNodeIds.reserve(static_cast<usize>(3 * numToDrop));
    for(int64 i = 0; i < nTriangle; i++)
    {
      const Triangle& triangle = triangles[static_cast<usize>(i)];
      if(SkipBackgroundSkinFace(triangle))
      {
        droppedNodeIds.push_back(triangle.node_id[0]);
        droppedNodeIds.push_back(triangle.node_id[1]);
        droppedNodeIds.push_back(triangle.node_id[2]);
      }
      else
      {
        triangles[static_cast<usize>(survivingCount)] = triangle;
        mCubeID[static_cast<usize>(survivingCount)] = mCubeID[static_cast<usize>(i)];
        survivingCount++;
      }
    }
    triangles.resize(static_cast<usize>(survivingCount));
    mCubeID.resize(static_cast<usize>(survivingCount));
    numFacesPruned = nTriangle - survivingCount;

    // Clear nodeType only for nodes the prune itself orphaned: referenced by a DROPPED triangle and
    // by no SURVIVING triangle. Nodes referenced only by survivors are left untouched, and
    // "pre-existing" candidates that no triangle -- dropped or surviving -- ever referenced are left
    // exactly as they were; the option no longer sweeps up orphan candidates it had no hand in creating.
    if(!droppedNodeIds.empty())
    {
      std::sort(droppedNodeIds.begin(), droppedNodeIds.end());
      droppedNodeIds.erase(std::unique(droppedNodeIds.begin(), droppedNodeIds.end()), droppedNodeIds.end());

      std::vector<bool> referencedBySurvivor(droppedNodeIds.size(), false);
      for(const auto& triangle : triangles)
      {
        for(const SiteId nodeId : triangle.node_id)
        {
          const auto it = std::lower_bound(droppedNodeIds.begin(), droppedNodeIds.end(), nodeId);
          if(it != droppedNodeIds.end() && *it == nodeId)
          {
            referencedBySurvivor[static_cast<usize>(it - droppedNodeIds.begin())] = true;
          }
        }
      }
      for(usize i = 0; i < droppedNodeIds.size(); i++)
      {
        if(!referencedBySurvivor[i])
        {
          nodeType[static_cast<usize>(droppedNodeIds[i])] = M3CNodeType::k_Unused;
        }
      }
    }
  }

  const int64 nTriangleFinal = static_cast<int64>(triangles.size());

  // Promote surface nodes to their exterior variant (+10). A triangle that borders the outside of the
  // volume has exactly one negative feature label (nSpin[0]*nSpin[1] < 0), so each of its nodes lies on
  // the volume boundary. This is the only output-relevant effect of the legacy triangle-side/inner-edge
  // connectivity pass: the per-triangle edge ids, edgePlace flags, and unique inner-edge list it also
  // built never appear in the output (Triangle Geometry + Face Labels + Node Types), so that machinery
  // has been removed.
  for(int64 j = 0; j < nTriangleFinal; j++)
  {
    if(triangles[j].nSpin[0] * triangles[j].nSpin[1] < 0)
    {
      for(int i = 0; i < 3; i++)
      {
        const SiteId tn = triangles[j].node_id[i];
        if(nodeType[tn] < 10)
        {
          nodeType[tn] = static_cast<int8>(nodeType[tn] + 10);
        }
      }
    }
  }

  // Release face edges before memory-heavy output and winding stages.
  std::vector<Segment>().swap(fedges);

  if(shouldCancel)
  {
    return {};
  }

  messageHandler("Writing surface mesh...");
  // A block prefix compacts real nodes without a dense candidate-to-id map.
  // Surface promotion never removes a node, so the sweep's node set is stable.
  const SiteId numCandidateNodes = 7 * numSites;
  constexpr SiteId k_NodeBlock = 128;
  const SiteId numNodeBlocks = (numCandidateNodes + k_NodeBlock - 1) / k_NodeBlock;
  std::vector<uint32> nodeBlockBase(static_cast<usize>(numNodeBlocks));
  int64 realNodeRunning = 0;
  for(SiteId b = 0; b < numNodeBlocks; b++)
  {
    nodeBlockBase[static_cast<usize>(b)] = static_cast<uint32>(realNodeRunning);
    const SiteId lo = b * k_NodeBlock;
    const SiteId hi = std::min<SiteId>(lo + k_NodeBlock, numCandidateNodes);
    for(SiteId c = lo; c < hi; c++)
    {
      if(nodeType[c] > 0)
      {
        realNodeRunning++;
      }
    }
  }
  const int64 nNodes = realNodeRunning;
  const auto compactedNodeId = [&](SiteId c) -> int64 {
    int64 r = nodeBlockBase[static_cast<usize>(c / k_NodeBlock)];
    for(SiteId cc = (c / k_NodeBlock) * k_NodeBlock; cc < c; cc++)
    {
      if(nodeType[cc] > 0)
      {
        r++;
      }
    }
    return r;
  };

  auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(inputValues->TriangleGeometryPath);
  triangleGeom.resizeVertexList(static_cast<usize>(nNodes));
  triangleGeom.resizeFaceList(static_cast<usize>(nTriangleFinal));
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nNodes)});
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({static_cast<usize>(nTriangleFinal)});

  auto& vertexStore = triangleGeom.getVertices()->getDataStoreRef();
  auto& triStore = triangleGeom.getFaces()->getDataStoreRef();
  auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(inputValues->FaceLabelsDataPath).getDataStoreRef();
  auto& nodeTypesOut = dataStructure.getDataRefAs<Int8Array>(inputValues->NodeTypesDataPath).getDataStoreRef();
  faceLabels.resizeTuples({static_cast<usize>(nTriangleFinal)});
  nodeTypesOut.resizeTuples({static_cast<usize>(nNodes)});

  // Emit real candidates in ascending order. This order preserves the legacy
  // compact node numbering without a candidate-to-node map.
  int64 vtxRunning = 0;
  for(SiteId i = 0; i < numCandidateNodes; i++)
  {
    if(nodeType[i] > 0)
    {
      const Node nodeCoord = nodeCoords[i];
      vertexStore[static_cast<usize>(vtxRunning) * 3 + 0] = nodeCoord.coord[0];
      vertexStore[static_cast<usize>(vtxRunning) * 3 + 1] = nodeCoord.coord[1];
      vertexStore[static_cast<usize>(vtxRunning) * 3 + 2] = nodeCoord.coord[2];
      nodeTypesOut[static_cast<usize>(vtxRunning)] = nodeType[i];
      vtxRunning++;
    }
  }

  // FaceLabels matches QuickSurfaceMesh and SurfaceNets. Negative ghost labels
  // become -1, and the reserved zero label becomes 0. The smaller label is first
  // because downstream filters require this order. Winding repair uses the same order.
  const auto toFaceLabel = [maxGrainId](int nSpin) -> int32 { return (nSpin < 0) ? -1 : ((nSpin == maxGrainId) ? 0 : nSpin); };

  // Triangles: remap to compacted node ids and write the ordered FaceLabels.
  for(int64 i = 0; i < nTriangleFinal; i++)
  {
    triStore[static_cast<usize>(i) * 3 + 0] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[0]));
    triStore[static_cast<usize>(i) * 3 + 1] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[1]));
    triStore[static_cast<usize>(i) * 3 + 2] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[2]));

    const int32 labelA = toFaceLabel(triangles[i].nSpin[0]);
    const int32 labelB = toFaceLabel(triangles[i].nSpin[1]);
    faceLabels[static_cast<usize>(i) * 2 + 0] = (labelA <= labelB) ? labelA : labelB;
    faceLabels[static_cast<usize>(i) * 2 + 1] = (labelA <= labelB) ? labelB : labelA;
  }

  // Transfer selected arrays to both face sides. Each side uses a source cell
  // whose working label matches that side. TupleTransfer skips exterior sides.
  if(!inputValues->SelectedCellDataArrayPaths.empty() || !inputValues->SelectedFeatureDataArrayPaths.empty())
  {
    messageHandler("Transferring attribute arrays to the mesh faces...");
    std::vector<std::shared_ptr<AbstractTupleTransfer>> transfers;
    for(usize i = 0; i < inputValues->SelectedCellDataArrayPaths.size(); i++)
    {
      AddTupleTransferInstance(dataStructure, inputValues->SelectedCellDataArrayPaths[i], inputValues->CreatedDataArrayPaths[i], transfers);
    }
    const usize numCellArrays = inputValues->SelectedCellDataArrayPaths.size();
    for(usize i = 0; i < inputValues->SelectedFeatureDataArrayPaths.size(); i++)
    {
      AddFeatureTupleTransferInstance(dataStructure, inputValues->SelectedFeatureDataArrayPaths[i], inputValues->CreatedDataArrayPaths[numCellArrays + i], inputValues->FeatureIdsArrayPath, transfers);
    }

    for(int64 i = 0; i < nTriangleFinal; i++)
    {
      // Use the FaceLabels order so each transferred component aligns with its label.
      const int32 labelA = toFaceLabel(triangles[i].nSpin[0]);
      const int32 labelB = toFaceLabel(triangles[i].nSpin[1]);
      const bool side0IsComp0 = (labelA <= labelB);
      const int nSpinComp0 = side0IsComp0 ? triangles[i].nSpin[0] : triangles[i].nSpin[1];
      const int nSpinComp1 = side0IsComp0 ? triangles[i].nSpin[1] : triangles[i].nSpin[0];
      const usize cell0 = findSourceCell(nSpinComp0, mCubeID[i], neighbors, point.data(), fileDim, dims);
      const usize cell1 = findSourceCell(nSpinComp1, mCubeID[i], neighbors, point.data(), fileDim, dims);
      for(const auto& transfer : transfers)
      {
        transfer->quickSurfaceTransfer(static_cast<usize>(i), cell0, cell1, faceLabels);
      }
    }
  }

  // Winding repair reads only the output geometry and FaceLabels. Release working
  // buffers before adjacency allocation to reduce peak memory.
  std::vector<Triangle>().swap(triangles);
  std::vector<SiteId>().swap(mCubeID);
  std::vector<int8>().swap(nodeType);
  std::vector<int32>().swap(point);
  std::vector<uint32>().swap(nodeBlockBase);

  // M3C does not guarantee globally consistent normals. Optional repair uses
  // triangle connectivity to make winding consistent with FaceLabels.
  if(inputValues->RepairTriangleWinding)
  {
    messageHandler("Generating connectivity and triangle neighbors...");
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(optionalId.has_value())
    {
      const auto& connectivity = dataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());
      messageHandler("Repairing windings...");
      Result<> windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                                       dataStructure.getDataAs<Int32Array>(inputValues->FaceLabelsDataPath)->getDataStoreRef(), shouldCancel, messageHandler);
      dataStructure.removeData(triangleGeom.getElementContainingVertId().value());
      dataStructure.removeData(triangleGeom.getElementNeighborsId().value());
      if(windingResult.invalid())
      {
        return windingResult;
      }
    }
  }

  if(inputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    // An entirely-background volume has nothing but {-1, 0} faces, so omitting the skin
    // legitimately produces an empty mesh. Report it rather than returning silently. Unlike
    // QuickSurfaceMesh/SurfaceNets, M3C's narrowed orphan-node clearing (see above) can leave
    // pre-existing candidate nodes in the output even when every face is dropped, so nNodes here
    // is not necessarily zero.
    if(nTriangleFinal == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(inputValues->TriangleGeometryPath, dataStructure.getDataRefAs<Int32Array>(inputValues->FeatureIdsArrayPath).getNumberOfTuples(),
                                                    static_cast<usize>(nNodes));
    }
    // A fully-indexed volume (no Feature Id 0) makes the option a no-op: nothing was pruned, and
    // the user otherwise gets byte-identical output with no feedback that the option had no effect.
    if(numFacesPruned == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(inputValues->TriangleGeometryPath);
    }
  }

  return {};
}
} // namespace

namespace nx::core
{
M3CSurfaceMeshing::M3CSurfaceMeshing(DataStructure& dataStructure, M3CSurfaceMeshingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

M3CSurfaceMeshing::~M3CSurfaceMeshing() noexcept = default;

Result<> M3CSurfaceMeshing::operator()()
{
  // Every dynamic cell and mesh array selects the residency path. A selected
  // input or created output can be disk-backed while Feature Ids remain resident.
  std::vector<const IArray*> dispatchTargets;
  const auto appendArray = [this, &dispatchTargets](const DataPath& path) {
    if(const auto* array = m_DataStructure.getDataAs<IDataArray>(path); array != nullptr)
    {
      dispatchTargets.push_back(array);
    }
  };
  appendArray(m_InputValues->FeatureIdsArrayPath);
  appendArray(m_InputValues->NodeTypesDataPath);
  appendArray(m_InputValues->FaceLabelsDataPath);
  for(const auto& path : m_InputValues->SelectedCellDataArrayPaths)
  {
    appendArray(path);
  }
  for(const auto& path : m_InputValues->CreatedDataArrayPaths)
  {
    appendArray(path);
  }
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  dispatchTargets.push_back(triangleGeom.getVertices());
  dispatchTargets.push_back(triangleGeom.getFaces());

  const bool usesOutOfCoreStore = AnyOutOfCore(AlgorithmArrayTargets(dispatchTargets));
  const bool useOutOfCorePath = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCorePath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, true, m_ShouldCancel, m_MessageHandler);
  if(sentinelCheck.invalid())
  {
    return sentinelCheck;
  }

  if(useOutOfCorePath)
  {
    return runOutOfCore(dispatchTargets, usesOutOfCoreStore);
  }

  // Default: the multithreaded sliding-window sweep (runWindowed(parallel=true)). Peak per-site scratch
  // is O(sliceArea) instead of O(volume), and the per-cube work runs across all cores. It is watertight
  // and correct, with byte-identical vertices, FaceLabels, and NodeTypes to the serial path, but a
  // slightly different (still valid) triangulation of the same interfaces -- the legacy per-cube loop
  // triangulation depends on cross-cube edge-flip propagation, which is inherently serial. The parallel
  // output is deterministic (each cube depends only on its own inputs, independent of thread scheduling).
  //
  // Two serial reference paths are kept for validation/debugging, selected via environment variables:
  //   M3C_SERIAL=1        -> runWindowed(false): serial sliding window (same tessellation as legacy)
  //   M3C_WHOLE_VOLUME=1  -> runEntireVolume():  serial whole-volume (O(volume) memory)
  // Both serial paths are byte-identical to each other.

  if(const char* wholeVol = std::getenv("M3C_WHOLE_VOLUME"); wholeVol != nullptr && std::string_view(wholeVol) == "1")
  {
    return runEntireVolume();
  }
  if(const char* serial = std::getenv("M3C_SERIAL"); serial != nullptr && std::string_view(serial) == "1")
  {
    return runWindowed(false);
  }
  return runWindowed(true);
}

Result<> M3CSurfaceMeshing::runOutOfCore(const std::vector<const IArray*>& dispatchTargets, bool usesOutOfCoreStore)
{
  if(dispatchTargets.empty())
  {
    return MakeErrorResult(-90544, "M3C out-of-core dispatch did not receive any dynamic storage targets.");
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const SizeVec3 gridDims = imageGeom.getDimensions();
  const usize dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0 || dims[0] > std::numeric_limits<usize>::max() / dims[1] || dims[0] * dims[1] > std::numeric_limits<usize>::max() / dims[2])
  {
    return MakeErrorResult(-90546, "M3C out-of-core input dimensions are zero or overflow the cell count.");
  }
  const usize cellCount = dims[0] * dims[1] * dims[2];
  if(featureIdsStore.getNumberOfTuples() != cellCount)
  {
    return MakeErrorResult(-90547, "M3C out-of-core FeatureIds tuple count does not match the Image Geometry.");
  }

  // First bounded pass preserves initialize_micro's zero-feature renumbering
  // without retaining a second copy of the cell data. Large bounded batches
  // are heap-backed so this path remains within the default Windows stack.
  constexpr usize kFeatureIdBulkValues = 65536;
  std::vector<int32> maxScanBuffer(kFeatureIdBulkValues);
  int32 maxGrainId = 0;
  for(usize offset = 0; offset < cellCount;)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(kFeatureIdBulkValues, cellCount - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(maxScanBuffer.data(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize index = 0; index < count; index++)
    {
      maxGrainId = std::max(maxGrainId, maxScanBuffer[index]);
    }
    offset += count;
  }
  if(maxGrainId == std::numeric_limits<int32>::max())
  {
    return MakeErrorResult(-90548, "M3C out-of-core FeatureIds maximum cannot be incremented to reserve the zero feature.");
  }
  maxGrainId++;

  const usize fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  if(fileDim[0] < dims[0] || fileDim[1] < dims[1] || fileDim[2] < dims[2] || fileDim[0] > std::numeric_limits<usize>::max() / fileDim[1] ||
     fileDim[0] * fileDim[1] > std::numeric_limits<usize>::max() / fileDim[2])
  {
    return MakeErrorResult(-90549, "M3C out-of-core padded dimensions overflow.");
  }
  if(fileDim[0] > static_cast<usize>(std::numeric_limits<int>::max()) || fileDim[0] * fileDim[1] > static_cast<usize>(std::numeric_limits<SiteId>::max()) ||
     fileDim[0] * fileDim[1] * fileDim[2] > static_cast<usize>(std::numeric_limits<SiteId>::max()))
  {
    return MakeErrorResult(-90550, "M3C out-of-core padded dimensions cannot be represented by its signed site/index arithmetic.");
  }
  const SiteId numSites = static_cast<SiteId>(fileDim[0] * fileDim[1] * fileDim[2]);
  const SiteId numSitesPerPlane = static_cast<SiteId>(fileDim[0] * fileDim[1]);
  if(numSites > std::numeric_limits<SiteId>::max() / 7 || numSites > std::numeric_limits<SiteId>::max() / 3)
  {
    return MakeErrorResult(-90551, "M3C out-of-core square or candidate-node count overflows its site index type.");
  }

  // Four LRU Z slices cover the 26-neighbor anomaly lookup while keeping the
  // padded ghost shell implicit. The cache also handles the NeighborAccessor's
  // toroidal border indices without materializing a padded volume.
  const usize sourceSliceSize = dims[0] * dims[1];
  std::array<std::vector<int32>, 4> sourceSlices;
  std::array<int64, 4> sourceSliceZ = {-1, -1, -1, -1};
  std::array<uint64, 4> sourceSliceUse{};
  uint64 sourceUseCounter = 0;
  const auto sourceValue = [&](SiteId site) -> Result<int32> {
    const usize linear = static_cast<usize>(site - 1);
    const usize x = linear % fileDim[0];
    const usize y = (linear / fileDim[0]) % fileDim[1];
    const usize z = linear / (fileDim[0] * fileDim[1]);
    if(z == 0 || z + 1 == fileDim[2] || y == 0 || y + 1 == fileDim[1] || x == 0 || x + 1 == fileDim[0])
    {
      return {k_GhostLabel};
    }
    const int64 sourceZ = static_cast<int64>(z - 1);
    usize slot = 0;
    while(slot < sourceSlices.size() && sourceSliceZ[slot] != sourceZ)
    {
      slot++;
    }
    if(slot == sourceSlices.size())
    {
      slot = static_cast<usize>(std::min_element(sourceSliceUse.begin(), sourceSliceUse.end()) - sourceSliceUse.begin());
      try
      {
        sourceSlices[slot].resize(sourceSliceSize);
      } catch(const std::bad_alloc&)
      {
        return MakeErrorResult<int32>(-90551, "M3C out-of-core rolling FeatureIds slice allocation failed.");
      }
      const usize sourceOffset = static_cast<usize>(sourceZ) * sourceSliceSize;
      auto readResult = featureIdsStore.copyIntoBuffer(sourceOffset, nonstd::span<int32>(sourceSlices[slot].data(), sourceSliceSize));
      if(readResult.invalid())
      {
        return ConvertInvalidResult<int32>(std::move(readResult));
      }
      sourceSliceZ[slot] = sourceZ;
    }
    sourceSliceUse[slot] = ++sourceUseCounter;
    const int32 value = sourceSlices[slot][(y - 1) * dims[0] + (x - 1)];
    return {value == 0 ? maxGrainId : value};
  };

  const NeighborAccessor neighbors{numSites, numSitesPerPlane, static_cast<int>(fileDim[0])};
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();
  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {spacing[0], spacing[1], spacing[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const uint64 candidateCount = static_cast<uint64>(7 * numSites);
  const SiteId lastCube = numSites - numSitesPerPlane;
  if(lastCube < 0 || static_cast<uint64>(lastCube) == std::numeric_limits<uint64>::max())
  {
    return MakeErrorResult(-90552, "M3C out-of-core cube-count record range overflows.");
  }
  const uint64 cubeRecordCount = static_cast<uint64>(lastCube) + 1;
  auto candidateResult = TemporaryRecordVector<M3CCandidateNodeRecord>::Create(candidateCount, usesOutOfCoreStore, m_ShouldCancel);
  if(candidateResult.invalid())
  {
    return ConvertResult(std::move(candidateResult));
  }
  auto triangleCountResult = TemporaryRecordVector<int64>::Create(cubeRecordCount, usesOutOfCoreStore, m_ShouldCancel);
  if(triangleCountResult.invalid())
  {
    return ConvertResult(std::move(triangleCountResult));
  }
  auto candidateNodes = std::move(candidateResult.value());
  auto triangleCounts = std::move(triangleCountResult.value());
  const M3CCandidateNodeRecord unusedNode{};
  auto fillNodesResult = candidateNodes.store().fill(0, candidateCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&unusedNode), sizeof(unusedNode)), m_ShouldCancel);
  if(fillNodesResult.invalid())
  {
    return fillNodesResult;
  }
  const int64 zeroTriangleCount = 0;
  auto fillCountsResult =
      triangleCounts.store().fill(0, cubeRecordCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&zeroTriangleCount), sizeof(zeroTriangleCount)), m_ShouldCancel);
  if(fillCountsResult.invalid())
  {
    return fillCountsResult;
  }

  const auto setNodeType = [&](SiteId nodeId, int8 type) -> Result<> {
    auto nodeResult = candidateNodes.cache().read(static_cast<uint64>(nodeId), m_ShouldCancel);
    if(nodeResult.invalid())
    {
      return ConvertResult(std::move(nodeResult));
    }
    auto node = nodeResult.value();
    node.type = type;
    return candidateNodes.cache().write(static_cast<uint64>(nodeId), node, m_ShouldCancel);
  };

  // Reconstruct one marching square in fixed local storage. The edge ids are
  // local to the caller; only the candidate-node classification survives pass
  // one and it lives in the external record vector.
  const auto buildSquare = [&](SiteId squareId, Face& square, std::array<Segment, 64>& segments, int& segmentCount, bool writeNodeTypes) -> Result<> {
    square = {};
    for(auto& edge : square.edge_id)
    {
      edge = k_UnusedNodeId;
    }
    square.FCnode = -1;
    const SiteId cubeOrigin = squareId / 3 + 1;
    const int squareOrder = static_cast<int>(squareId % 3);
    const auto corners = squareCorners(squareId, neighbors);
    int spins[4];
    int ghostCorners = 0;
    for(int index = 0; index < 4; index++)
    {
      auto spinResult = sourceValue(corners[index]);
      if(spinResult.invalid())
      {
        return ConvertResult(std::move(spinResult));
      }
      spins[index] = spinResult.value();
      ghostCorners += spins[index] < 0 ? 1 : 0;
    }
    if(ghostCorners != 4)
    {
      square.effect = 1;
    }
    if(ghostCorners == 4)
    {
      return {};
    }
    int squareIndex = get_square_index(spins);
    if(squareIndex == 15)
    {
      int neighborCounts[4] = {0, 0, 0, 0};
      for(int corner = 0; corner < 4; corner++)
      {
        const Neighbor cornerNeighbors = neighbors[corners[corner]];
        for(int neighborIndex = 1; neighborIndex <= num_neigh; neighborIndex++)
        {
          auto neighborSpin = sourceValue(cornerNeighbors.neigh_id[neighborIndex]);
          if(neighborSpin.invalid())
          {
            return ConvertResult(std::move(neighborSpin));
          }
          neighborCounts[corner] += spins[corner] == neighborSpin.value() && neighborSpin.value() > 0 ? 1 : 0;
        }
      }
      int minimum = 1000;
      int minimumIndex = -1;
      for(int corner = 0; corner < 4; corner++)
      {
        if(neighborCounts[corner] < minimum)
        {
          minimum = neighborCounts[corner];
          minimumIndex = corner;
        }
      }
      squareIndex += minimumIndex == 0 || minimumIndex == 2 ? 1 : 0;
    }
    if(squareIndex == 0)
    {
      return {};
    }
    for(int edgeIndex = 0; edgeIndex < 8; edgeIndex += 2)
    {
      if(k_EdgeTable2d[squareIndex][edgeIndex] == -1)
      {
        continue;
      }
      const int nodeIndex[2] = {k_EdgeTable2d[squareIndex][edgeIndex], k_EdgeTable2d[squareIndex][edgeIndex + 1]};
      const int pixelIndex[2] = {k_NsTable2d[squareIndex][edgeIndex], k_NsTable2d[squareIndex][edgeIndex + 1]};
      SiteId nodeIds[2];
      get_nodes(cubeOrigin, squareOrder, nodeIndex, nodeIds, numSitesPerPlane, static_cast<int>(fileDim[0]));
      const int pixelSpins[2] = {spins[pixelIndex[0]], spins[pixelIndex[1]]};
      if(pixelSpins[0] > 0 || pixelSpins[1] > 0)
      {
        if(segmentCount >= static_cast<int>(segments.size()))
        {
          return MakeErrorResult(-90552, "M3C out-of-core local square edge buffer overflowed.");
        }
        segments[static_cast<usize>(segmentCount)] = Segment{{nodeIds[0], nodeIds[1]}, {pixelSpins[0], pixelSpins[1]}};
        square.edge_id[square.nEdge++] = static_cast<uint32>(segmentCount++);
      }
      else if(writeNodeTypes)
      {
        auto firstResult = setNodeType(nodeIds[0], M3CNodeType::k_Unused);
        if(firstResult.invalid())
        {
          return firstResult;
        }
        auto secondResult = setNodeType(nodeIds[1], M3CNodeType::k_Unused);
        if(secondResult.invalid())
        {
          return secondResult;
        }
      }
      for(int node = 0; node < 2; node++)
      {
        if(nodeIndex[node] == 4 && (squareIndex == 7 || squareIndex == 11 || squareIndex == 13 || squareIndex == 14 || squareIndex == 19))
        {
          square.FCnode = nodeIds[node];
        }
        if(writeNodeTypes)
        {
          int8 type = M3CNodeType::k_Default;
          if(nodeIndex[node] == 4)
          {
            if(squareIndex == 19)
            {
              type = M3CNodeType::k_QuadPoint;
            }
            else if(squareIndex == 7 || squareIndex == 11 || squareIndex == 13 || squareIndex == 14)
            {
              type = M3CNodeType::k_TriplePoint;
            }
            else
            {
              continue;
            }
          }
          auto typeResult = setNodeType(nodeIds[node], type);
          if(typeResult.invalid())
          {
            return typeResult;
          }
        }
      }
    }
    return {};
  };

  // Match the legacy edge-stage visitation order when classifying
  // candidate nodes. No resident square/edge vector survives this pass.
  for(SiteId squareId = 0; squareId < 3 * numSites; squareId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    Face square;
    std::array<Segment, 64> segments{};
    int segmentCount = 0;
    auto squareResult = buildSquare(squareId, square, segments, segmentCount, true);
    if(squareResult.invalid())
    {
      return squareResult;
    }
  }

  // A cube references only six squares and at most 24 face edges, so its
  // count is rebuilt locally. Count records preserve the default parallel
  // path's cube indexing/order for the later prefix + generation pass.
  uint64 numFacesPruned = 0;
  uint64 maximumTrianglesPerCube = 0;
  const auto skipBackgroundSkinFace = [maxGrainId](const Triangle& triangle) {
    const int spinA = triangle.nSpin[0];
    const int spinB = triangle.nSpin[1];
    return (spinA < 0 && spinB == maxGrainId) || (spinB < 0 && spinA == maxGrainId);
  };
  for(SiteId cube = 1; cube <= lastCube; cube++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const SiteId squareIds[6] = {3 * (cube - 1), 3 * (cube - 1) + 1, 3 * (cube - 1) + 2, 3 * cube + 2, 3 * (cube + static_cast<SiteId>(fileDim[0]) - 1) + 1, 3 * (cube + numSitesPerPlane - 1)};
    std::array<Face, 6> squares{};
    std::array<Segment, 64> segments{};
    int segmentCount = 0;
    SiteId faceCenters[6] = {-1, -1, -1, -1, -1, -1};
    int faceCenterCount = 0;
    int edgeCount = 0;
    int effectiveCount = 0;
    for(int square = 0; square < 6; square++)
    {
      auto squareResult = buildSquare(squareIds[square], squares[square], segments, segmentCount, false);
      if(squareResult.invalid())
      {
        return squareResult;
      }
      if(squares[square].FCnode != -1)
      {
        faceCenters[faceCenterCount++] = squares[square].FCnode;
      }
      edgeCount += squares[square].nEdge;
      effectiveCount += squares[square].effect;
    }
    if(faceCenterCount >= 3)
    {
      const auto firstCorners = squareCorners(squareIds[0], neighbors);
      const auto lastCorners = squareCorners(squareIds[5], neighbors);
      int uniqueSpins = 0;
      int cubeSpins[8];
      for(int index = 0; index < 4; index++)
      {
        auto firstSpin = sourceValue(firstCorners[index]);
        auto lastSpin = sourceValue(lastCorners[index]);
        if(firstSpin.invalid() || lastSpin.invalid())
        {
          return firstSpin.invalid() ? ConvertResult(std::move(firstSpin)) : ConvertResult(std::move(lastSpin));
        }
        cubeSpins[index] = firstSpin.value();
        cubeSpins[index + 4] = lastSpin.value();
      }
      for(int index = 0; index < 8; index++)
      {
        const int spin = cubeSpins[index];
        if(spin != -1)
        {
          uniqueSpins++;
          cubeSpins[index] = -1;
          for(int other = 0; other < 8; other++)
          {
            if(cubeSpins[other] == spin)
            {
              cubeSpins[other] = -1;
            }
          }
        }
      }
      auto bodyResult = setNodeType(7 * (cube - 1) + 6, static_cast<int8>(std::min(uniqueSpins, static_cast<int>(M3CNodeType::k_QuadPoint))));
      if(bodyResult.invalid())
      {
        return bodyResult;
      }
    }
    int64 count = 0;
    if(effectiveCount > 0 && edgeCount > 2)
    {
      std::array<SiteId, 64> edgeIds{};
      int edgeIndex = 0;
      for(const auto& square : squares)
      {
        for(int index = 0; index < square.nEdge; index++)
        {
          edgeIds[edgeIndex++] = square.edge_id[index];
        }
      }
      if(faceCenterCount == 0)
      {
        count = get_number_case0_triangles(edgeIds.data(), segments.data(), edgeCount);
      }
      else if(faceCenterCount == 2)
      {
        count = get_number_case2_triangles(edgeIds.data(), segments.data(), edgeCount, faceCenters, faceCenterCount);
      }
      else if(faceCenterCount > 2 && faceCenterCount <= 6)
      {
        count = get_number_caseM_triangles(edgeIds.data(), segments.data(), edgeCount, faceCenters, faceCenterCount);
      }
      maximumTrianglesPerCube = std::max(maximumTrianglesPerCube, static_cast<uint64>(count));

      if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && count > 0)
      {
        std::vector<Triangle> countTriangles(static_cast<usize>(count));
        std::vector<SiteId> countCubes(static_cast<usize>(count));
        double c1[3];
        double c2[3];
        for(int component = 0; component < 3; component++)
        {
          c1[component] = siteCoords[cube].coord[component];
          c2[component] = siteCoords[cube + 1 + static_cast<SiteId>(fileDim[0]) + numSitesPerPlane].coord[component];
        }
        int64 generatedCount = 0;
        if(faceCenterCount == 0)
        {
          get_case0_triangles(countTriangles.data(), countCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, 0, &generatedCount, c1, c2, cube);
        }
        else if(faceCenterCount == 2)
        {
          get_case2_triangles(countTriangles.data(), countCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, faceCenters, faceCenterCount, 0, &generatedCount, c1, c2, cube);
        }
        else
        {
          get_caseM_triangles(countTriangles.data(), countCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, faceCenters, faceCenterCount, 0, &generatedCount, 7 * (cube - 1) + 6,
                              c1, c2, cube);
        }
        if(generatedCount != count)
        {
          return MakeErrorResult(-90559, "M3C out-of-core pruning generation disagrees with its counted triangle range.");
        }

        int64 survivingCount = 0;
        for(const Triangle& triangle : countTriangles)
        {
          const bool dropTriangle = skipBackgroundSkinFace(triangle);
          const uint8 referenceFlag = dropTriangle ? uint8{1} : uint8{2};
          for(const SiteId nodeId : triangle.node_id)
          {
            auto nodeResult = candidateNodes.cache().read(static_cast<uint64>(nodeId), m_ShouldCancel);
            if(nodeResult.invalid())
            {
              return ConvertResult(std::move(nodeResult));
            }
            auto node = nodeResult.value();
            node.pruneReferences = static_cast<uint8>(node.pruneReferences | referenceFlag);
            auto writeResult = candidateNodes.cache().write(static_cast<uint64>(nodeId), node, m_ShouldCancel);
            if(writeResult.invalid())
            {
              return writeResult;
            }
          }
          if(dropTriangle)
          {
            numFacesPruned++;
          }
          else
          {
            survivingCount++;
          }
        }
        count = survivingCount;
      }
    }
    auto writeResult = triangleCounts.cache().write(static_cast<uint64>(cube), count, m_ShouldCancel);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  auto flushNodesResult = candidateNodes.flush(m_ShouldCancel);
  if(flushNodesResult.invalid())
  {
    return flushNodesResult;
  }
  auto flushCountsResult = triangleCounts.flush(m_ShouldCancel);
  if(flushCountsResult.invalid())
  {
    return flushCountsResult;
  }

  // Convert counts in place to the 1-based deterministic cube offsets used by
  // the parallel path. The values remain in external storage for pass 2.
  uint64 triangleTotal = 0;
  for(SiteId cube = 1; cube <= lastCube; cube++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto countResult = triangleCounts.cache().read(static_cast<uint64>(cube), m_ShouldCancel);
    if(countResult.invalid())
    {
      return ConvertResult(std::move(countResult));
    }
    const int64 count = countResult.value();
    if(count < 0 || static_cast<uint64>(count) > static_cast<uint64>(std::numeric_limits<int64>::max()) - triangleTotal ||
       triangleTotal > static_cast<uint64>(std::numeric_limits<usize>::max()) - static_cast<uint64>(count))
    {
      return MakeErrorResult(-90553, "M3C out-of-core triangle count or offset overflows its output range.");
    }
    auto offsetResult = triangleCounts.cache().write(static_cast<uint64>(cube), static_cast<int64>(triangleTotal), m_ShouldCancel);
    if(offsetResult.invalid())
    {
      return offsetResult;
    }
    triangleTotal += static_cast<uint64>(count);
  }
  auto offsetFlushResult = triangleCounts.flush(m_ShouldCancel);
  if(offsetFlushResult.invalid())
  {
    return offsetFlushResult;
  }

  // Candidate IDs compact in ascending candidate order, exactly matching the
  // original assign_new_nodeID traversal. Vertex/NodeTypes output waits until
  // generation has marked exterior nodes.
  uint64 nodeTotal = 0;
  for(uint64 candidate = 0; candidate < candidateCount; candidate++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto nodeResult = candidateNodes.cache().read(candidate, m_ShouldCancel);
    if(nodeResult.invalid())
    {
      return ConvertResult(std::move(nodeResult));
    }
    auto node = nodeResult.value();
    bool nodeChanged = false;
    if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && node.pruneReferences == uint8{1} && node.type > 0)
    {
      node.type = M3CNodeType::k_Unused;
      nodeChanged = true;
    }
    if(node.type > 0)
    {
      if(nodeTotal == std::numeric_limits<uint64>::max() || nodeTotal >= static_cast<uint64>(std::numeric_limits<usize>::max()))
      {
        return MakeErrorResult(-90554, "M3C out-of-core compacted node count overflows its output range.");
      }
      node.compactId = nodeTotal++;
      nodeChanged = true;
    }
    if(nodeChanged)
    {
      auto writeResult = candidateNodes.cache().write(candidate, node, m_ShouldCancel);
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
  }
  auto compactFlushResult = candidateNodes.flush(m_ShouldCancel);
  if(compactFlushResult.invalid())
  {
    return compactFlushResult;
  }

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  triangleGeom.resizeVertexList(static_cast<usize>(nodeTotal));
  triangleGeom.resizeFaceList(static_cast<usize>(triangleTotal));
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeTotal)});
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({static_cast<usize>(triangleTotal)});
  auto& faceStore = triangleGeom.getFaces()->getDataStoreRef();
  auto& faceLabelsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsDataPath).getDataStoreRef();
  auto& nodeTypesStore = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesDataPath).getDataStoreRef();
  faceLabelsStore.resizeTuples({static_cast<usize>(triangleTotal)});
  nodeTypesStore.resizeTuples({static_cast<usize>(nodeTotal)});

  std::vector<std::shared_ptr<AbstractTupleTransfer>> transfers;
  for(usize index = 0; index < m_InputValues->SelectedCellDataArrayPaths.size(); index++)
  {
    AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[index], m_InputValues->CreatedDataArrayPaths[index], transfers);
  }
  const usize cellArrayCount = m_InputValues->SelectedCellDataArrayPaths.size();
  for(usize index = 0; index < m_InputValues->SelectedFeatureDataArrayPaths.size(); index++)
  {
    AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[index], m_InputValues->CreatedDataArrayPaths[cellArrayCount + index],
                                    m_InputValues->FeatureIdsArrayPath, transfers);
  }
  const auto outputLabel = [maxGrainId](int spin) { return spin < 0 ? int32{-1} : (spin == maxGrainId ? int32{0} : static_cast<int32>(spin)); };
  const auto sourceCell = [&](int label, SiteId cube) -> Result<usize> {
    const Neighbor n = neighbors[cube];
    const SiteId corners[8] = {cube, n.neigh_id[1], n.neigh_id[7], n.neigh_id[8], n.neigh_id[18], n.neigh_id[19], n.neigh_id[25], n.neigh_id[26]};
    for(SiteId site : corners)
    {
      auto value = sourceValue(site);
      if(value.invalid())
      {
        return ConvertInvalidResult<usize>(std::move(value));
      }
      if(value.value() == label)
      {
        const usize linear = static_cast<usize>(site - 1);
        const usize x = linear % fileDim[0];
        const usize y = (linear / fileDim[0]) % fileDim[1];
        const usize z = linear / (fileDim[0] * fileDim[1]);
        if(x >= 1 && x <= dims[0] && y >= 1 && y <= dims[1] && z >= 1 && z <= dims[2])
        {
          return {(z - 1) * sourceSliceSize + (y - 1) * dims[0] + (x - 1)};
        }
      }
    }
    return {std::numeric_limits<usize>::max()};
  };

  constexpr usize kFaceBatch = 16384;
  std::vector<IGeometry::MeshIndexType> faceValues(kFaceBatch * 3);
  std::vector<int32> labelValues(kFaceBatch * 2);
  std::vector<QuickSurfaceTransferData> transferValues(kFaceBatch);
  const usize localCapacity = std::max<usize>(1, static_cast<usize>(maximumTrianglesPerCube));
  std::vector<Triangle> localTriangles(localCapacity);
  std::vector<SiteId> localCubes(localCapacity);
  for(SiteId cube = 1; cube <= lastCube; cube++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto offset = triangleCounts.cache().read(static_cast<uint64>(cube), m_ShouldCancel);
    if(offset.invalid())
    {
      return ConvertResult(std::move(offset));
    }
    const usize destination = static_cast<usize>(offset.value());
    const SiteId squareIds[6] = {3 * (cube - 1), 3 * (cube - 1) + 1, 3 * (cube - 1) + 2, 3 * cube + 2, 3 * (cube + static_cast<SiteId>(fileDim[0]) - 1) + 1, 3 * (cube + numSitesPerPlane - 1)};
    std::array<Face, 6> squares{};
    std::array<Segment, 64> segments{};
    std::array<SiteId, 64> edgeIds{};
    SiteId centers[6] = {-1, -1, -1, -1, -1, -1};
    int segmentCount = 0;
    int edgeCount = 0;
    int centerCount = 0;
    int effectiveCount = 0;
    for(int square = 0; square < 6; square++)
    {
      auto result = buildSquare(squareIds[square], squares[square], segments, segmentCount, false);
      if(result.invalid())
      {
        return result;
      }
      if(squares[square].FCnode != -1)
      {
        centers[centerCount++] = squares[square].FCnode;
      }
      effectiveCount += squares[square].effect;
      for(int edge = 0; edge < squares[square].nEdge; edge++)
      {
        edgeIds[edgeCount++] = squares[square].edge_id[edge];
      }
    }
    usize generated = 0;
    if(effectiveCount > 0 && edgeCount > 2)
    {
      double c1[3];
      double c2[3];
      for(int component = 0; component < 3; component++)
      {
        c1[component] = siteCoords[cube].coord[component];
        c2[component] = siteCoords[cube + 1 + static_cast<SiteId>(fileDim[0]) + numSitesPerPlane].coord[component];
      }
      int64 end = 0;
      if(centerCount == 0)
      {
        get_case0_triangles(localTriangles.data(), localCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, 0, &end, c1, c2, cube);
      }
      else if(centerCount == 2)
      {
        get_case2_triangles(localTriangles.data(), localCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, centers, centerCount, 0, &end, c1, c2, cube);
      }
      else if(centerCount > 2)
      {
        get_caseM_triangles(localTriangles.data(), localCubes.data(), edgeIds.data(), nodeCoords, segments.data(), edgeCount, centers, centerCount, 0, &end, 7 * (cube - 1) + 6, c1, c2, cube);
      }
      generated = static_cast<usize>(end);
    }
    if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
    {
      usize survivingCount = 0;
      for(usize index = 0; index < generated; index++)
      {
        if(!skipBackgroundSkinFace(localTriangles[index]))
        {
          localTriangles[survivingCount] = localTriangles[index];
          localCubes[survivingCount] = localCubes[index];
          survivingCount++;
        }
      }
      generated = survivingCount;
    }
    uint64 expectedEnd = triangleTotal;
    if(cube != lastCube)
    {
      auto nextOffset = triangleCounts.cache().read(static_cast<uint64>(cube + 1), m_ShouldCancel);
      if(nextOffset.invalid() || nextOffset.value() < 0)
      {
        return nextOffset.invalid() ? ConvertResult(std::move(nextOffset)) : MakeErrorResult(-90558, "M3C out-of-core triangle offset is negative.");
      }
      expectedEnd = static_cast<uint64>(nextOffset.value());
    }
    if(generated > localCapacity || expectedEnd < static_cast<uint64>(destination) || generated != expectedEnd - static_cast<uint64>(destination))
    {
      return MakeErrorResult(-90555, "M3C out-of-core generation disagrees with its counted triangle range.");
    }
    for(usize start = 0; start < generated; start += kFaceBatch)
    {
      const usize count = std::min(kFaceBatch, generated - start);
      for(usize local = 0; local < count; local++)
      {
        const Triangle& triangle = localTriangles[start + local];
        for(int vertex = 0; vertex < 3; vertex++)
        {
          auto nodeResult = candidateNodes.cache().read(static_cast<uint64>(triangle.node_id[vertex]), m_ShouldCancel);
          if(nodeResult.invalid() || nodeResult.value().type <= 0)
          {
            return nodeResult.invalid() ? ConvertResult(std::move(nodeResult)) : MakeErrorResult(-90556, "M3C out-of-core triangle references an unused candidate node.");
          }
          auto node = nodeResult.value();
          if((triangle.nSpin[0] < 0) != (triangle.nSpin[1] < 0) && node.type < 10)
          {
            node.type = static_cast<int8>(node.type + 10);
            auto write = candidateNodes.cache().write(static_cast<uint64>(triangle.node_id[vertex]), node, m_ShouldCancel);
            if(write.invalid())
            {
              return write;
            }
          }
          faceValues[local * 3 + vertex] = static_cast<IGeometry::MeshIndexType>(node.compactId);
        }
        const int32 a = outputLabel(triangle.nSpin[0]);
        const int32 b = outputLabel(triangle.nSpin[1]);
        const bool aFirst = a <= b;
        labelValues[local * 2] = aFirst ? a : b;
        labelValues[local * 2 + 1] = aFirst ? b : a;
        auto first = sourceCell(aFirst ? triangle.nSpin[0] : triangle.nSpin[1], cube);
        auto second = sourceCell(aFirst ? triangle.nSpin[1] : triangle.nSpin[0], cube);
        if(first.invalid() || second.invalid())
        {
          return first.invalid() ? ConvertResult(std::move(first)) : ConvertResult(std::move(second));
        }
        transferValues[local] = {destination + start + local, first.value(), second.value(), labelValues[local * 2], labelValues[local * 2 + 1]};
      }
      auto faceWrite = faceStore.copyFromBuffer((destination + start) * 3, nonstd::span<const IGeometry::MeshIndexType>(faceValues.data(), count * 3));
      auto labelWrite = faceLabelsStore.copyFromBuffer((destination + start) * 2, nonstd::span<const int32>(labelValues.data(), count * 2));
      if(faceWrite.invalid() || labelWrite.invalid())
      {
        return faceWrite.invalid() ? faceWrite : labelWrite;
      }
      for(const auto& transfer : transfers)
      {
        auto result = transfer->quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData>(transferValues.data(), count));
        if(result.invalid())
        {
          return result;
        }
      }
    }
  }
  auto promotionFlush = candidateNodes.flush(m_ShouldCancel);
  if(promotionFlush.invalid())
  {
    return promotionFlush;
  }

  auto& vertexStore = triangleGeom.getVertices()->getDataStoreRef();
  constexpr usize kVertexBatch = 16384;
  std::vector<float32> vertexValues(kVertexBatch * 3);
  std::vector<int8> typeValues(kVertexBatch);
  for(uint64 candidate = 0; candidate < candidateCount;)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const uint64 end = std::min(candidateCount, candidate + static_cast<uint64>(kVertexBatch));
    usize count = 0;
    uint64 firstCompact = 0;
    for(uint64 current = candidate; current < end; current++)
    {
      auto nodeResult = candidateNodes.cache().read(current, m_ShouldCancel);
      if(nodeResult.invalid())
      {
        return ConvertResult(std::move(nodeResult));
      }
      const auto& node = nodeResult.value();
      if(node.type > 0)
      {
        if(count == 0)
        {
          firstCompact = node.compactId;
        }
        const Node coordinate = nodeCoords[static_cast<SiteId>(current)];
        vertexValues[count * 3] = coordinate.coord[0];
        vertexValues[count * 3 + 1] = coordinate.coord[1];
        vertexValues[count * 3 + 2] = coordinate.coord[2];
        typeValues[count++] = node.type;
      }
    }
    if(count > 0)
    {
      auto vertexWrite = vertexStore.copyFromBuffer(firstCompact * 3, nonstd::span<const float32>(vertexValues.data(), count * 3));
      auto typeWrite = nodeTypesStore.copyFromBuffer(firstCompact, nonstd::span<const int8>(typeValues.data(), count));
      if(vertexWrite.invalid() || typeWrite.invalid())
      {
        return vertexWrite.invalid() ? vertexWrite : typeWrite;
      }
    }
    candidate = end;
  }
  if(m_InputValues->RepairTriangleWinding)
  {
    auto& ioCollection = DataStoreUtilities::GetIOCollection();
    if(ioCollection.hasExternalSortCapability() && ioCollection.hasTemporaryRecordStoreCapability())
    {
      auto result = MeshingUtilities::RepairTriangleWindingExternal(faceStore, faceLabelsStore, m_ShouldCancel, m_MessageHandler);
      if(result.invalid())
      {
        return result;
      }
    }
    else if(usesOutOfCoreStore)
    {
      return MakeErrorResult(-90557, "M3C out-of-core winding repair requires external-sort and temporary-record-store providers.");
    }
    else
    {
      triangleGeom.findElementNeighbors(true);
      const auto optionalId = triangleGeom.getElementNeighborsId();
      if(optionalId.has_value())
      {
        const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());
        auto result = MeshingUtilities::RepairTriangleWinding(faceStore, connectivity, faceLabelsStore, m_ShouldCancel, m_MessageHandler);
        m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
        m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
        if(result.invalid())
        {
          return result;
        }
      }
    }
  }
  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    if(triangleTotal == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, featureIds.getNumberOfTuples(), static_cast<usize>(nodeTotal));
    }
    if(numFacesPruned == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(m_InputValues->TriangleGeometryPath);
    }
  }
  return {};
}

Result<> M3CSurfaceMeshing::runEntireVolume()
{
  // M3C node coordinates require the uniform spacing of ImageGeom.
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();

  SizeVec3 gridDims = imageGeom.getDimensions();
  usize dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 imgOrigin = imageGeom.getOrigin();
  const float res[3] = {spacing[0], spacing[1], spacing[2]};
  const float origin[3] = {imgOrigin[0], imgOrigin[1], imgOrigin[2]};

  // NX inputs do not include an exterior layer. Add one for surface closure.
  constexpr bool k_AddSurfaceLayer = true;
  usize fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  const usize totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  // SiteId supports grids with more than 2^31 voxels. The 32-bit edge and node
  // storage still limits mesh size to approximately 2^32 elements.
  const SiteId numSites = static_cast<SiteId>(totalPoints);
  const SiteId numSitesPerPlane = static_cast<SiteId>(fileDim[0] * fileDim[1]);

  // Read Feature Ids directly into the padded grid. Zero-label renumbering changes
  // only this working copy.
  m_MessageHandler("Initializing working grid and ghost layer...");
  std::vector<int32> point(totalPoints + 1, 0);
  const int maxGrainId = initialize_micro(k_AddSurfaceLayer, dims, fileDim, featureIdsStore, point.data());

  // Calculate coordinates and neighbors on demand to avoid three full-volume arrays.
  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {res[0], res[1], res[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const NeighborAccessor neighbors{numSites, numSitesPerPlane, static_cast<int>(fileDim[0])};

  // Each site owns top, back, and left squares. Seven candidate node types start unused.
  m_MessageHandler("Initializing candidate nodes and squares...");
  std::vector<Face> squares(static_cast<usize>(3) * numSites);
  std::vector<int8> nodeType(static_cast<usize>(7) * numSites, 0);
  initialize_squares(squares.data(), numSites);

  if(m_ShouldCancel)
  {
    return {};
  }

  // Count face edges before their exact allocation.
  m_MessageHandler("Counting face edges...");
  const int64 nFEdge = get_number_fEdges(squares.data(), point.data(), neighbors, numSites, m_ShouldCancel);

  m_MessageHandler("Finding nodes and edges on each square...");
  std::vector<Segment> fedges(static_cast<usize>(nFEdge < 0 ? 0 : nFEdge));
  get_nodes_fEdges(squares.data(), point.data(), neighbors, nodeType.data(), fedges.data(), numSites, numSitesPerPlane, static_cast<int>(fileDim[0]), m_ShouldCancel);

  if(m_ShouldCancel)
  {
    return {};
  }

  // Count triangles before their exact allocation.
  m_MessageHandler("Counting triangles...");
  const int64 nTriangle = get_number_triangles(point.data(), squares.data(), neighbors, nodeType.data(), fedges.data(), numSites, numSitesPerPlane, static_cast<int>(fileDim[0]), m_ShouldCancel);

  m_MessageHandler("Generating triangles...");
  std::vector<Triangle> triangles(static_cast<usize>(nTriangle < 0 ? 0 : nTriangle));
  std::vector<SiteId> mCubeID(static_cast<usize>(nTriangle < 0 ? 0 : nTriangle), 0);
  get_triangles(siteCoords, triangles.data(), mCubeID.data(), squares.data(), nodeCoords, fedges.data(), numSites, numSitesPerPlane, static_cast<int>(fileDim[0]), m_ShouldCancel);

  if(m_ShouldCancel)
  {
    return {};
  }

  return finalizeMesh(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel, triangles, mCubeID, fedges, nodeType, point, nodeCoords, neighbors, numSites, fileDim, dims, maxGrainId);
}

Result<> M3CSurfaceMeshing::runWindowed(bool parallel)
{
  // The sliding window keeps marching-square scratch to two Z slices. Serial
  // execution matches runEntireVolume. Parallel execution can change triangulation.
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();

  SizeVec3 gridDims = imageGeom.getDimensions();
  usize dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 imgOrigin = imageGeom.getOrigin();
  const float res[3] = {spacing[0], spacing[1], spacing[2]};
  const float origin[3] = {imgOrigin[0], imgOrigin[1], imgOrigin[2]};

  constexpr bool k_AddSurfaceLayer = true;
  usize fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  const usize totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  const SiteId numSites = static_cast<SiteId>(totalPoints);
  const SiteId numSitesPerPlane = static_cast<SiteId>(fileDim[0] * fileDim[1]);
  const int xDim = static_cast<int>(fileDim[0]);

  m_MessageHandler("Initializing working grid and ghost layer...");
  std::vector<int32> point(totalPoints + 1, 0);
  const int maxGrainId = initialize_micro(k_AddSurfaceLayer, dims, fileDim, featureIdsStore, point.data());

  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {res[0], res[1], res[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const NeighborAccessor neighbors{numSites, numSitesPerPlane, xDim};

  // Mesh-scale vectors grow with output size. Only per-site squares are windowed.
  // Face edges reserve once from an early rate estimate. Triangles and cube IDs
  // resize once after the first pass counts their exact size.
  std::vector<int8> nodeType(static_cast<usize>(7) * numSites, 0);
  std::vector<Segment> fedges;
  std::vector<Triangle> triangles;
  std::vector<SiteId> mCubeID;

  // The square window holds two Z slices. It slides one slice as the cube sweep
  // advances and keeps absolute square IDs mapped to local slots.
  const SiteId sliceSquares = 3 * numSitesPerPlane; // squares per z-slice
  std::vector<Face> window(static_cast<usize>(2) * sliceSquares);
  SiteId winBaseSite = 1;
  auto winIndex = [&winBaseSite](SiteId squareId) -> usize { return static_cast<usize>(squareId - 3 * (winBaseSite - 1)); };

  // Build square edges and node types in ascending square order. This preserves
  // global face-edge IDs and the legacy effect flag.
  auto computeSquares = [&](SiteId kLo, SiteId kHi, bool appendEdges, int64& eid) {
    for(SiteId k = kLo; k < kHi; k++)
    {
      Face& sqk = window[winIndex(k)];
      for(int j = 0; j < 4; j++)
      {
        sqk.edge_id[j] = k_UnusedNodeId;
      }
      sqk.nEdge = 0;
      sqk.FCnode = -1;
      sqk.effect = 0;

      const SiteId cubeOrigin = k / 3 + 1;
      const int sqOrder = static_cast<int>(k % 3);
      const std::array<SiteId, 4> tnsite = squareCorners(k, neighbors);
      int tnspin[4];
      int numGhostCorners = 0;
      for(int m = 0; m < 4; m++)
      {
        tnspin[m] = point[tnsite[m]];
        if(tnspin[m] < 0)
        {
          numGhostCorners++;
        }
      }
      if(numGhostCorners != 4)
      {
        sqk.effect = 1;
      }

      int edgeCount = 0;
      if(numGhostCorners != 4)
      {
        int sqIndex = get_square_index(tnspin);
        if(sqIndex == 15)
        {
          sqIndex = sqIndex + treat_anomaly(tnsite, point.data(), neighbors, k);
        }
        if(sqIndex != 0)
        {
          for(int j = 0; j < 8; j = j + 2)
          {
            if(k_EdgeTable2d[sqIndex][j] != -1)
            {
              int nodeIndex[2] = {k_EdgeTable2d[sqIndex][j], k_EdgeTable2d[sqIndex][j + 1]};
              int pixIndex[2] = {k_NsTable2d[sqIndex][j], k_NsTable2d[sqIndex][j + 1]};
              SiteId nodeID[2];
              int pixSpin[2];
              get_nodes(cubeOrigin, sqOrder, nodeIndex, nodeID, numSitesPerPlane, xDim);
              get_spins(point.data(), cubeOrigin, sqOrder, pixIndex, pixSpin, numSitesPerPlane, xDim);

              if(pixSpin[0] > 0 || pixSpin[1] > 0)
              {
                Segment seg;
                seg.node_id[0] = nodeID[0];
                seg.node_id[1] = nodeID[1];
                seg.nSpin[0] = pixSpin[0];
                seg.nSpin[1] = pixSpin[1];
                sqk.edge_id[edgeCount] = static_cast<uint32>(eid);
                if(appendEdges)
                {
                  fedges.push_back(seg);
                }
                eid++;
                edgeCount++;
              }
              else
              {
                nodeType[nodeID[0]] = M3CNodeType::k_Unused;
                nodeType[nodeID[1]] = M3CNodeType::k_Unused;
              }

              for(int ii = 0; ii < 2; ii++)
              {
                if(nodeIndex[ii] == 4)
                {
                  if(sqIndex == 7 || sqIndex == 11 || sqIndex == 13 || sqIndex == 14)
                  {
                    SiteId tnode = nodeID[ii];
                    sqk.FCnode = tnode;
                    nodeType[tnode] = M3CNodeType::k_TriplePoint;
                  }
                  else if(sqIndex == 19)
                  {
                    SiteId tnode = nodeID[ii];
                    sqk.FCnode = tnode;
                    nodeType[tnode] = M3CNodeType::k_QuadPoint;
                  }
                }
                else
                {
                  // Interior edge endpoints must remain real nodes after compaction.
                  SiteId tnode = nodeID[ii];
                  nodeType[tnode] = M3CNodeType::k_Default;
                }
              }
            }
          }
        }
      }
      sqk.nEdge = edgeCount;
    }
  };

  // Case functions flip shared face edges while tracing loops. Count every cube
  // before generating any triangle to preserve the serial ordering.
  int64 nTriangle = 0;

  const int64 totalSlices = (numSitesPerPlane > 0) ? (numSites / numSitesPerPlane) : 1;
  const int64 progressStep = std::max<int64>(1, totalSlices / 20); // ~20 progress updates per sweep

  // Estimate face-edge capacity after early slices. One reservation avoids repeated
  // multi-gigabyte reallocations and transient memory spikes.
  bool fedgesReserved = false;
  const int64 reserveAfterSlices = std::max<int64>(4, totalSlices / 16);
  auto maybeReserveFedges = [&](int64 eid) {
    const int64 sliceIdx = winBaseSite / numSitesPerPlane;
    if(fedgesReserved || sliceIdx < reserveAfterSlices || sliceIdx >= totalSlices)
    {
      return;
    }
    fedgesReserved = true;
    // The window has computed two slices beyond the slide counter.
    const int64 slicesComputed = sliceIdx + 2;
    const auto projected = static_cast<usize>(static_cast<double>(eid) / static_cast<double>(slicesComputed) * static_cast<double>(totalSlices) * 1.05);
    if(projected > fedges.capacity())
    {
      fedges.reserve(projected);
    }
  };

  auto sweep = [&](bool appendEdges, bool generate) {
    winBaseSite = 1;
    int64 eid = 0;
    int64 tidRun = 0;
    computeSquares(0, std::min<SiteId>(2 * sliceSquares, 3 * numSites), appendEdges, eid);

    for(SiteId i = 1; i <= (numSites - numSitesPerPlane); i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Slide the window to cover the current and next site planes.
      while(i >= winBaseSite + numSitesPerPlane)
      {
        std::memmove(window.data(), window.data() + sliceSquares, static_cast<usize>(sliceSquares) * sizeof(Face));
        winBaseSite += numSitesPerPlane;
        const int64 sliceIdx = winBaseSite / numSitesPerPlane;
        if(sliceIdx % progressStep == 0)
        {
          m_MessageHandler(fmt::format("Sweeping z-slices ({}): slice {} / {}", generate ? "pass 2, generating triangles" : "pass 1, counting", sliceIdx, totalSlices));
        }
        const SiteId newLoSquare = 3 * (winBaseSite + numSitesPerPlane - 1);
        const SiteId newHiSquare = std::min<SiteId>(3 * (winBaseSite + 2 * numSitesPerPlane - 1), 3 * numSites);
        if(newLoSquare < newHiSquare)
        {
          computeSquares(newLoSquare, newHiSquare, appendEdges, eid);
        }
        if(appendEdges)
        {
          maybeReserveFedges(eid);
        }
      }

      SiteId sqID[6];
      sqID[0] = 3 * (i - 1);
      sqID[1] = 3 * (i - 1) + 1;
      sqID[2] = 3 * (i - 1) + 2;
      sqID[3] = 3 * i + 2;
      sqID[4] = 3 * (i + xDim - 1) + 1;
      sqID[5] = 3 * (i + numSitesPerPlane - 1);

      SiteId arrayFC[6];
      for(int ii = 0; ii < 6; ii++)
      {
        arrayFC[ii] = -1;
      }
      int fcid = 0;
      int nFE = 0;
      int eff = 0;
      for(int ii = 0; ii < 6; ii++)
      {
        const Face& sqf = window[winIndex(sqID[ii])];
        if(sqf.FCnode != -1)
        {
          arrayFC[fcid] = sqf.FCnode;
          fcid++;
        }
        nFE = nFE + sqf.nEdge;
        eff = eff + sqf.effect;
      }
      const int nFC = fcid;
      const int cubeFlag = (eff > 0) ? 1 : 0;
      const SiteId BCnode = 7 * (i - 1) + 6;

      // The count pass assigns the body-center type when three or more face
      // centers meet. This timing matches the whole-volume path.
      if(!generate && nFC >= 3)
      {
        const std::array<SiteId, 4> corners1 = squareCorners(sqID[0], neighbors);
        const std::array<SiteId, 4> corners2 = squareCorners(sqID[5], neighbors);
        int arraySpin[8];
        for(int j = 0; j < 4; j++)
        {
          arraySpin[j] = point[corners1[j]];
          arraySpin[j + 4] = point[corners2[j]];
        }
        int nds = 0;
        for(int k = 0; k < 8; k++)
        {
          int cspin = arraySpin[k];
          if(cspin != -1)
          {
            nds++;
            arraySpin[k] = -1;
            for(int kk = 0; kk < 8; kk++)
            {
              if(cspin == arraySpin[kk])
              {
                arraySpin[kk] = -1;
              }
            }
          }
        }
        // NodeType uses k_QuadPoint for four or more labels.
        nodeType[BCnode] = static_cast<int8>(std::min(nds, static_cast<int>(M3CNodeType::k_QuadPoint)));
      }

      if(cubeFlag != 1 || nFE <= 2)
      {
        continue;
      }

      std::vector<SiteId> arrayFE(nFE);
      int tindex = 0;
      for(int i1 = 0; i1 < 6; i1++)
      {
        const Face& sqf = window[winIndex(sqID[i1])];
        int tnfe = sqf.nEdge;
        for(int i2 = 0; i2 < tnfe; i2++)
        {
          arrayFE[tindex] = sqf.edge_id[i2];
          tindex++;
        }
      }

      if(!generate)
      {
        // The first pass counts triangles and applies whole-volume edge flips.
        if(nFC == 0)
        {
          nTriangle += get_number_case0_triangles(arrayFE.data(), fedges.data(), nFE);
        }
        else if(nFC == 2)
        {
          nTriangle += get_number_case2_triangles(arrayFE.data(), fedges.data(), nFE, arrayFC, nFC);
        }
        else if(nFC > 2 && nFC <= 6)
        {
          nTriangle += get_number_caseM_triangles(arrayFE.data(), fedges.data(), nFE, arrayFC, nFC);
        }
        continue;
      }

      // The second pass writes triangles to the pre-sized arrays in cube order.
      double coord1[3];
      double coord2[3];
      for(int k = 0; k < 3; k++)
      {
        coord1[k] = siteCoords[i].coord[k];
        coord2[k] = siteCoords[i + 1 + xDim + numSitesPerPlane].coord[k];
      }
      int64 tin = tidRun;
      int64 tout = tin;
      if(nFC == 0)
      {
        get_case0_triangles(triangles.data(), mCubeID.data(), arrayFE.data(), nodeCoords, fedges.data(), nFE, tin, &tout, coord1, coord2, i);
      }
      else if(nFC == 2)
      {
        get_case2_triangles(triangles.data(), mCubeID.data(), arrayFE.data(), nodeCoords, fedges.data(), nFE, arrayFC, nFC, tin, &tout, coord1, coord2, i);
      }
      else
      {
        get_caseM_triangles(triangles.data(), mCubeID.data(), arrayFE.data(), nodeCoords, fedges.data(), nFE, arrayFC, nFC, tin, &tout, BCnode, coord1, coord2, i);
      }
      tidRun = tout;
    }
  };

  if(!parallel)
  {
    m_MessageHandler("Sweeping z-slices (pass 1: face edges + triangle count)...");
    sweep(true, false);
    if(m_ShouldCancel)
    {
      return {};
    }
    triangles.resize(static_cast<usize>(nTriangle < 0 ? 0 : nTriangle));
    mCubeID.resize(static_cast<usize>(nTriangle < 0 ? 0 : nTriangle), 0);

    m_MessageHandler("Sweeping z-slices (pass 2: generating triangles)...");
    sweep(false, true);
  }
  else
  {
    // The serial edge stage preserves vertices, labels, and node types. Parallel
    // cubes read shared squares and flip private edge copies. This avoids shared
    // mutation. Cross-cube flips are omitted, so triangulation can differ while
    // interfaces remain valid and watertight.
    const SiteId lastCube = numSites - numSitesPerPlane;
    const usize numCubes = (lastCube >= 1) ? static_cast<usize>(lastCube) : 0; // cubes are 1..lastCube

    // Each cube uses private edges. Counting sets body-center node types. Generation
    // writes triangles at the precomputed offset.
    auto perCube = [&](SiteId i, bool doGenerate, int64 triOffset) -> int64 {
      SiteId sqID[6];
      sqID[0] = 3 * (i - 1);
      sqID[1] = 3 * (i - 1) + 1;
      sqID[2] = 3 * (i - 1) + 2;
      sqID[3] = 3 * i + 2;
      sqID[4] = 3 * (i + xDim - 1) + 1;
      sqID[5] = 3 * (i + numSitesPerPlane - 1);
      SiteId arrayFC[6];
      for(int ii = 0; ii < 6; ii++)
      {
        arrayFC[ii] = -1;
      }
      int fcid = 0;
      int nFE = 0;
      int eff = 0;
      for(int ii = 0; ii < 6; ii++)
      {
        const Face& sqf = window[winIndex(sqID[ii])];
        if(sqf.FCnode != -1)
        {
          arrayFC[fcid] = sqf.FCnode;
          fcid++;
        }
        nFE = nFE + sqf.nEdge;
        eff = eff + sqf.effect;
      }
      const int nFC = fcid;
      const SiteId BCnode = 7 * (i - 1) + 6;
      if(!doGenerate && nFC >= 3)
      {
        const std::array<SiteId, 4> corners1 = squareCorners(sqID[0], neighbors);
        const std::array<SiteId, 4> corners2 = squareCorners(sqID[5], neighbors);
        int arraySpin[8];
        for(int j = 0; j < 4; j++)
        {
          arraySpin[j] = point[corners1[j]];
          arraySpin[j + 4] = point[corners2[j]];
        }
        int nds = 0;
        for(int k = 0; k < 8; k++)
        {
          int cspin = arraySpin[k];
          if(cspin != -1)
          {
            nds++;
            arraySpin[k] = -1;
            for(int kk = 0; kk < 8; kk++)
            {
              if(cspin == arraySpin[kk])
              {
                arraySpin[kk] = -1;
              }
            }
          }
        }
        // NodeType uses k_QuadPoint for four or more labels.
        nodeType[BCnode] = static_cast<int8>(std::min(nds, static_cast<int>(M3CNodeType::k_QuadPoint)));
      }
      if(eff <= 0 || nFE <= 2)
      {
        return 0;
      }
      // Private face-edge indexes start at zero. A cube has at most 24 edges,
      // and the fixed local buffer holds 64.
      std::array<SiteId, 64> localAFE{};
      std::array<Segment, 64> localEdges{};
      int tindex = 0;
      for(int i1 = 0; i1 < 6; i1++)
      {
        const Face& sqf = window[winIndex(sqID[i1])];
        int tnfe = sqf.nEdge;
        for(int i2 = 0; i2 < tnfe; i2++)
        {
          localEdges[static_cast<usize>(tindex)] = fedges[sqf.edge_id[i2]];
          localAFE[static_cast<usize>(tindex)] = tindex;
          tindex++;
        }
      }
      if(!doGenerate)
      {
        if(nFC == 0)
        {
          return get_number_case0_triangles(localAFE.data(), localEdges.data(), nFE);
        }
        if(nFC == 2)
        {
          return get_number_case2_triangles(localAFE.data(), localEdges.data(), nFE, arrayFC, nFC);
        }
        if(nFC > 2 && nFC <= 6)
        {
          return get_number_caseM_triangles(localAFE.data(), localEdges.data(), nFE, arrayFC, nFC);
        }
        return 0;
      }
      double coord1[3];
      double coord2[3];
      for(int k = 0; k < 3; k++)
      {
        coord1[k] = siteCoords[i].coord[k];
        coord2[k] = siteCoords[i + 1 + xDim + numSitesPerPlane].coord[k];
      }
      int64 tin = triOffset;
      int64 tout = tin;
      if(nFC == 0)
      {
        get_case0_triangles(triangles.data(), mCubeID.data(), localAFE.data(), nodeCoords, localEdges.data(), nFE, tin, &tout, coord1, coord2, i);
      }
      else if(nFC == 2)
      {
        get_case2_triangles(triangles.data(), mCubeID.data(), localAFE.data(), nodeCoords, localEdges.data(), nFE, arrayFC, nFC, tin, &tout, coord1, coord2, i);
      }
      else
      {
        get_caseM_triangles(triangles.data(), mCubeID.data(), localAFE.data(), nodeCoords, localEdges.data(), nFE, arrayFC, nFC, tin, &tout, BCnode, coord1, coord2, i);
      }
      return tout - triOffset;
    };

    // Slide the serial edge window until it covers the target and next site planes.
    auto advanceWindowTo = [&](SiteId targetBaseSite, bool appendEdges, int64& eid) {
      while(winBaseSite < targetBaseSite)
      {
        std::memmove(window.data(), window.data() + sliceSquares, static_cast<usize>(sliceSquares) * sizeof(Face));
        winBaseSite += numSitesPerPlane;
        const SiteId newLoSquare = 3 * (winBaseSite + numSitesPerPlane - 1);
        const SiteId newHiSquare = std::min<SiteId>(3 * (winBaseSite + 2 * numSitesPerPlane - 1), 3 * numSites);
        if(newLoSquare < newHiSquare)
        {
          computeSquares(newLoSquare, newHiSquare, appendEdges, eid);
        }
        if(appendEdges)
        {
          maybeReserveFedges(eid);
        }
      }
    };

    std::vector<int64> triOffset(numCubes + 1, 0); // 1-based per-cube; pass 1 fills counts, then prefix -> offsets

    m_MessageHandler("Sweeping z-slices (parallel pass 1: counting)...");
    {
      winBaseSite = 1;
      int64 eid = 0;
      computeSquares(0, std::min<SiteId>(2 * sliceSquares, 3 * numSites), true, eid);
      for(SiteId sliceBase = 1; sliceBase <= lastCube; sliceBase += numSitesPerPlane)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        advanceWindowTo(sliceBase, true, eid);
        const SiteId cubeEnd = std::min<SiteId>(sliceBase + numSitesPerPlane, lastCube + 1);
        ParallelDataAlgorithm alg;
        alg.setRange(static_cast<usize>(sliceBase), static_cast<usize>(cubeEnd));
        alg.execute([&](const Range& range) {
          for(usize idx = range.min(); idx < range.max(); idx++)
          {
            triOffset[idx] = perCube(static_cast<SiteId>(idx), false, 0);
          }
        });
      }
    }

    int64 total = 0;
    for(usize i = 1; i <= numCubes; i++)
    {
      const int64 cnt = triOffset[i];
      triOffset[i] = total;
      total += cnt;
    }
    triangles.resize(static_cast<usize>(total));
    mCubeID.resize(static_cast<usize>(total), 0);

    m_MessageHandler("Sweeping z-slices (parallel pass 2: generating triangles)...");
    {
      winBaseSite = 1;
      int64 eid = 0;
      computeSquares(0, std::min<SiteId>(2 * sliceSquares, 3 * numSites), false, eid);
      for(SiteId sliceBase = 1; sliceBase <= lastCube; sliceBase += numSitesPerPlane)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        advanceWindowTo(sliceBase, false, eid);
        const SiteId cubeEnd = std::min<SiteId>(sliceBase + numSitesPerPlane, lastCube + 1);
        ParallelDataAlgorithm alg;
        alg.setRange(static_cast<usize>(sliceBase), static_cast<usize>(cubeEnd));
        alg.execute([&](const Range& range) {
          for(usize idx = range.min(); idx < range.max(); idx++)
          {
            perCube(static_cast<SiteId>(idx), true, triOffset[idx]);
          }
        });
      }
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  return finalizeMesh(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel, triangles, mCubeID, fedges, nodeType, point, nodeCoords, neighbors, numSites, fileDim, dims, maxGrainId);
}
} // namespace nx::core
