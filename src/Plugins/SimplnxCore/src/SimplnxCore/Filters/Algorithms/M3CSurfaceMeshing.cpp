#include "M3CSurfaceMeshing.hpp"

#include "SimplnxCore/Filters/Algorithms/TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

// =============================================================================
// Port of the legacy DREAM3D M3CEntireVolume algorithm (all-in-memory variant).
// The multi-material marching-cubes core operates on flat, 1-based index arrays;
// the helpers below are transcribed closely from
//   DREAM3D/Source/Plugins/SurfaceMeshing/SurfaceMeshingFilters/Unsupported/M3CEntireVolume.cpp
// Only the thin operator() layer touches simplnx (read ImageGeom+FeatureIds,
// write TriangleGeom+FaceLabels+NodeTypes).
// =============================================================================
namespace
{
using int64 = std::int64_t;
using uint64 = std::uint64_t;

// Index of a padded site / voxel (i.e. an index into the FeatureId grid). MUST be 64-bit: a large
// Image Geometry can have well over 2^31 voxels, and node ids derived as 7*site must not overflow.
using SiteId = std::int64_t;
// Sentinel stored in the compacted-node map for candidate slots that are not real mesh nodes.
constexpr uint32_t k_UnusedNodeId = std::numeric_limits<uint32_t>::max();

constexpr int num_neigh = 26; // number of 3D neighbors per site (legacy #define)

// --- M3C working structs (mirror SIMPL/Geometry/MeshStructs.h SurfaceMesh::M3C) ---
struct Node
{
  float coord[3];
};
struct VoxelCoord
{
  float coord[3];
};
struct Neighbor
{
  SiteId neigh_id[27]; // 1-based; index 0 unused. 64-bit: these index the FeatureId grid.
};
struct Face // a marching "square"
{
  // The 4 corner site ids are NOT stored (they are recomputed on demand from the site index, keeping
  // the FeatureId indexing 64-bit) so that this, the largest working array (3 per site), stays small.
  // edge_id is an UNSIGNED 32-bit edge index (mesh-scale, capped near 2^32 edges); FCnode is a 64-bit node id.
  uint32_t edge_id[4];
  SiteId FCnode; // face-center node id, -1 if none
  int8_t nEdge;
  int8_t effect; // 0 = useless square, 1 = straddles >=2 labels
};
struct Segment // a face edge
{
  int64 node_id[2];
  int edgeKind;
  int nSpin[2]; // labels on left/right of the arrow
};
struct ISegment // an inner edge (connectivity)
{
  int64 node_id[2];
  int edgeKind;
  int nSpin[4];
};
struct Triangle
{
  int64 node_id[3];
  uint64 e_id[3];
  int nSpin[2];
  int edgePlace[3];
};

// On-demand coordinate accessors. A site's coordinate and each of its 7 candidate node positions are
// pure functions of the 1-based padded site index, so they are computed as needed rather than stored
// in full-volume arrays (which dominated the algorithm's memory footprint).
struct SiteCoords
{
  size_t fileDim0;
  size_t fileDim1;
  size_t fileNSP; // fileDim0 * fileDim1
  float res[3];
  float origin[3];

  VoxelCoord operator[](int64 site) const
  {
    const size_t linear = static_cast<size_t>(site - 1);
    // Subtract the 1-cell ghost shell so that real cell (0,0,0) at padded index (1,1,1) maps to the
    // geometry origin, keeping M3C's coordinates aligned with the input volume and the other meshers.
    const int64 i = static_cast<int64>(linear % fileDim0) - 1;
    const int64 j = static_cast<int64>((linear / fileDim0) % fileDim1) - 1;
    const int64 k = static_cast<int64>(linear / fileNSP) - 1;
    return VoxelCoord{{static_cast<float>(i) * res[0] + origin[0], static_cast<float>(j) * res[1] + origin[1], static_cast<float>(k) * res[2] + origin[2]}};
  }
};

struct NodeCoords
{
  SiteCoords sites;

  // 7 candidate nodes per site: +x/+y/+z edge midpoints (0,1,2), xy/xz/yz face centers (3,4,5),
  // body center (6). Matches the legacy initialize_nodes half-spacing offsets.
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
    case 0: n.coord[0] += hx; break;
    case 1: n.coord[1] += hy; break;
    case 2: n.coord[2] += hz; break;
    case 3: n.coord[0] += hx; n.coord[1] += hy; break;
    case 4: n.coord[0] += hx; n.coord[2] += hz; break;
    case 5: n.coord[1] += hy; n.coord[2] += hz; break;
    case 6: n.coord[0] += hx; n.coord[1] += hy; n.coord[2] += hz; break;
    default: break;
    }
    return n;
  }
};

// The multi-material marching-squares case tables. Byte-identical between the legacy
// M3CSliceBySlice.cpp and M3CEntireVolume.cpp (verified). 20 cases x up to 4 edges.
// edgeTable_2d: per case, node-slot pairs (0-3 edge midpoints, 4 = face-center) forming edges.
// nsTable_2d:   per edge, the pixel-slot pair whose FeatureIds become that edge's two labels.
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
// negative labels (-3..-8) when addSurfaceLayer is true, fill voxel coordinates,
// and renumber any FeatureId==0 to maxGrainId. Returns maxGrainId (the value that
// zeros were remapped to; callers revert it on output). Transcribed from
// M3CEntireVolume::initialize_micro_from_grainIds.
// -----------------------------------------------------------------------------
int initialize_micro(bool addSurfaceLayer, const size_t dims[3], const size_t fileDim[3], const AbstractDataStore<int32_t>& grainIds, int32_t* p)
{
  int maxGrainId = 0;

  if(!addSurfaceLayer)
  {
    size_t totalPoints = dims[0] * dims[1] * dims[2];
    for(size_t i = 0; i < totalPoints; ++i)
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
    size_t index = 0;
    size_t gIdx = 0;

    // Bottom wrapping slice
    for(size_t i = 0; i < (fileDim[0] * fileDim[1]); ++i)
    {
      p[++index] = -3;
    }
    // Bulk of the volume, wrapped per-plane and per-row
    for(size_t z = 0; z < dims[2]; ++z)
    {
      for(size_t i = 0; i < fileDim[0]; ++i)
      {
        p[++index] = -4;
      }
      for(size_t y = 0; y < dims[1]; ++y)
      {
        p[++index] = -5; // leading surface voxel for this row
        for(size_t x = 0; x < dims[0]; ++x)
        {
          p[++index] = grainIds[gIdx++];
          if(p[index] > maxGrainId)
          {
            maxGrainId = p[index];
          }
        }
        p[++index] = -6; // trailing surface voxel for this row
      }
      for(size_t i = 0; i < fileDim[0]; ++i)
      {
        p[++index] = -7;
      }
    }
    // Top wrapping slice
    for(size_t i = 0; i < (fileDim[0] * fileDim[1]); ++i)
    {
      p[++index] = -8;
    }
  }

  // Independent grain id for the (formerly) zero feature
  maxGrainId = maxGrainId + 1;

  p[0] = 0; // Point 0 is garbage

  // Renumber the (formerly) zero feature to the reserved id. Ghost cells are negative and untouched.
  // Voxel coordinates are no longer stored; they are computed on demand via SiteCoords/NodeCoords.
  const size_t totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  for(size_t id = 1; id <= totalPoints; id++)
  {
    if(p[id] == 0)
    {
      p[id] = maxGrainId;
    }
  }
  return maxGrainId;
}

// -----------------------------------------------------------------------------
// On-demand 26-neighbor accessor (replaces a stored per-site array, ~108 bytes/site). Reproduces the
// toroidal indexing of the legacy get_neighbor_list; the ghost shell makes the wrap harmless. Because
// operator[] recomputes all 26 ids, callers that read several neighbors of the same site should cache
// the returned Neighbor in a local rather than indexing the accessor repeatedly.
// -----------------------------------------------------------------------------
struct NeighborAccessor
{
  SiteId ns;
  SiteId nsp;
  int xDim;

  Neighbor operator[](SiteId site_id) const
  {
    // Recover the (k, j, i) that the legacy triple loop used for this 1-based site.
    const SiteId within = (site_id - 1) % nsp; // == j + (i - 1)
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

// The 4 corner site ids of a marching square (squareId = 3*(site-1) + orientation), recomputed on
// demand in place of the former Face::site_id storage. Mirrors the corners set by initialize_squares.
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

// -----------------------------------------------------------------------------
// (Candidate node coordinates are computed on demand via NodeCoords; node types are
//  zero-initialized with their backing vector, so no explicit node-initialization pass is needed.)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Initialize the 3 marching squares per site to their empty sentinels. Corner site ids are no longer
// stored (see squareCorners), so only the edge/flag fields are reset here.
// -----------------------------------------------------------------------------
void initialize_squares(Face* sq, SiteId ns)
{
  for(SiteId sqId = 0; sqId < 3 * ns; sqId++)
  {
    for(int j = 0; j < 4; j++)
    {
      sq[sqId].edge_id[j] = -1;
    }
    sq[sqId].nEdge = 0;
    sq[sqId].FCnode = -1;
    sq[sqId].effect = 0;
  }
}

// Node type values (match SIMPL::SurfaceMesh::NodeType / what simplnx LaplacianSmoothing consumes).
namespace M3CNodeType
{
constexpr int8_t k_Unused = 0;
constexpr int8_t k_Default = 2;
constexpr int8_t k_TriplePoint = 3;
constexpr int8_t k_QuadPoint = 4;
constexpr int8_t k_SurfaceDefault = 12;
constexpr int8_t k_SurfaceTriplePoint = 13;
constexpr int8_t k_SurfaceQuadPoint = 14;
} // namespace M3CNodeType

// -----------------------------------------------------------------------------
// Classify a square's 4 corner labels into cases 0..19. Transcribed from
// M3CEntireVolume::get_square_index.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Disambiguate the all-corners-differ saddle (case 15) using the 3D same-label
// neighbor counts. Transcribed from M3CEntireVolume::treat_anomaly.
// -----------------------------------------------------------------------------
int treat_anomaly(const std::array<SiteId, 4>& tnst, const int32_t* p1, const NeighborAccessor& n1, SiteId /*sqid*/)
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

// -----------------------------------------------------------------------------
// Map an edge-table node slot (0-4) for a given square order to a concrete
// candidate-node id. Transcribed from M3CEntireVolume::get_nodes.
// -----------------------------------------------------------------------------
void get_nodes(SiteId cst, int ord, const int nidx[2], SiteId* nid, SiteId nsp1, int xDim1)
{
  for(int ii = 0; ii < 2; ii++)
  {
    int tempIndex = nidx[ii];
    if(ord == 0)
    {
      switch(tempIndex)
      {
      case 0: nid[ii] = 7 * (cst - 1); break;
      case 1: nid[ii] = 7 * cst + 1; break;
      case 2: nid[ii] = 7 * (cst + xDim1 - 1); break;
      case 3: nid[ii] = 7 * (cst - 1) + 1; break;
      case 4: nid[ii] = 7 * (cst - 1) + 3; break;
      }
    }
    else if(ord == 1)
    {
      switch(tempIndex)
      {
      case 0: nid[ii] = 7 * (cst - 1); break;
      case 1: nid[ii] = 7 * cst + 2; break;
      case 2: nid[ii] = 7 * (cst + nsp1 - 1); break;
      case 3: nid[ii] = 7 * (cst - 1) + 2; break;
      case 4: nid[ii] = 7 * (cst - 1) + 4; break;
      }
    }
    else
    {
      switch(tempIndex)
      {
      case 0: nid[ii] = 7 * (cst - 1) + 1; break;
      case 1: nid[ii] = 7 * (cst - 1) + 2; break;
      case 2: nid[ii] = 7 * (cst + nsp1 - 1) + 1; break;
      case 3: nid[ii] = 7 * (cst + xDim1 - 1) + 2; break;
      case 4: nid[ii] = 7 * (cst - 1) + 5; break;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Map a square's two side-pixel slots to the two straddling FeatureIds.
// Transcribed from M3CEntireVolume::get_spins.
// -----------------------------------------------------------------------------
void get_spins(const int32_t* p1, SiteId cst, int ord, const int pID[2], int* pSpin, SiteId nsp1, int xDim1)
{
  for(int i = 0; i < 2; i++)
  {
    int pixTemp = pID[i];
    if(ord == 0)
    {
      switch(pixTemp)
      {
      case 0: pSpin[i] = p1[cst]; break;
      case 1: pSpin[i] = p1[cst + 1]; break;
      case 2: pSpin[i] = p1[cst + xDim1 + 1]; break;
      case 3: pSpin[i] = p1[cst + xDim1]; break;
      }
    }
    else if(ord == 1)
    {
      switch(pixTemp)
      {
      case 0: pSpin[i] = p1[cst]; break;
      case 1: pSpin[i] = p1[cst + 1]; break;
      case 2: pSpin[i] = p1[cst + nsp1 + 1]; break;
      case 3: pSpin[i] = p1[cst + nsp1]; break;
      }
    }
    else if(ord == 2)
    {
      switch(pixTemp)
      {
      case 0: pSpin[i] = p1[cst + xDim1]; break;
      case 1: pSpin[i] = p1[cst]; break;
      case 2: pSpin[i] = p1[cst + nsp1]; break;
      case 3: pSpin[i] = p1[cst + nsp1 + xDim1]; break;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Count the total number of face edges across all squares (two-pass sizing).
// Transcribed from M3CEntireVolume::get_number_fEdges.
// -----------------------------------------------------------------------------
int64 get_number_fEdges(Face* sq, const int32_t* p, const NeighborAccessor& n, SiteId ns, const std::atomic_bool& shouldCancel)
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
    int atBulk = 0;
    for(int m = 0; m < 4; m++)
    {
      tnspin[m] = p[tnsite[m]];
      if(tnspin[m] < 0)
      {
        atBulk++;
      }
    }
    if(atBulk != 4)
    {
      sq[k].effect = 1; // mark as effective (can be marching-cubed)
    }

    if(atBulk != 4)
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
        if(atBulk == 3)
        {
          numCEdge = 2;
        }
        else if(atBulk == 2)
        {
          numCEdge = 3;
        }
        else if(atBulk == 1)
        {
          // "one negative spin" case is not supposed to happen; leave numCEdge = 0.
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

// -----------------------------------------------------------------------------
// Emit the actual face-edge segments, record them on squares, and set node
// types (triple/quad on face centers, default elsewhere, unused on pure-surface
// edges). Transcribed from M3CEntireVolume::get_nodes_fEdges.
// -----------------------------------------------------------------------------
void get_nodes_fEdges(Face* sq, const int32_t* p, const NeighborAccessor& n, int8_t* nodeType, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
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
    int atBulk = 0;
    for(int m = 0; m < 4; m++)
    {
      tnspin[m] = p[tnsite[m]];
      if(tnspin[m] < 0)
      {
        atBulk++;
      }
    }

    int edgeCount = 0;
    if(atBulk != 4)
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
              sq[k].edge_id[edgeCount] = eid;
              e[eid].edgeKind = 2;
              edgeCount++;
              eid++;
            }
            else
            {
              // pure box-surface edge: mark its nodes unused
              nodeType[nodeID[0]] = M3CNodeType::k_Unused;
              nodeType[nodeID[1]] = M3CNodeType::k_Unused;
            }

            // Categorize the two nodes of this edge (triple/quad on face-center slot 4).
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
                SiteId tnode = nodeID[ii];
                if(nodeType[tnode] != -1)
                {
                  nodeType[tnode] = M3CNodeType::k_Default;
                }
              }
            }
          }
        }
      }
    }
    sq[k].nEdge = edgeCount;
  }
}

// -----------------------------------------------------------------------------
// Classify each triangle edge as face/inner. NOTE: legacy always sets tw[*]=1
// (both branches); the inner/outer distinction is effectively unused downstream.
// Transcribed from M3CEntireVolume::find_edgePlace.
// -----------------------------------------------------------------------------
void find_edgePlace(const double tvcrd1[3], const double tvcrd2[3], const double tvcrd3[3], int tw[3], double xh, double xl, double yh, double yl, double zh, double zl)
{
  const double eps = 1.0e-6;
  double txc, tyc, tzc;

  txc = (tvcrd1[0] + tvcrd2[0]) / 2.0;
  tyc = (tvcrd1[1] + tvcrd2[1]) / 2.0;
  tzc = (tvcrd1[2] + tvcrd2[2]) / 2.0;
  tw[0] = 1;
  (void)((txc < (xh - eps) && txc > (xl + eps)) && (tyc < (yh - eps) && tyc > (yl + eps)) && (tzc < (zh - eps) && tzc > (zl + eps)));

  txc = (tvcrd2[0] + tvcrd3[0]) / 2.0;
  tyc = (tvcrd2[1] + tvcrd3[1]) / 2.0;
  tzc = (tvcrd2[2] + tvcrd3[2]) / 2.0;
  tw[1] = 1;

  txc = (tvcrd3[0] + tvcrd1[0]) / 2.0;
  tyc = (tvcrd3[1] + tvcrd1[1]) / 2.0;
  tzc = (tvcrd3[2] + tvcrd1[2]) / 2.0;
  tw[2] = 1;
  (void)txc;
  (void)tyc;
  (void)tzc;
}

// -----------------------------------------------------------------------------
// Count triangles for a case-0 cube (no face centers): burn edges into closed
// loops, fan-triangulate. Transcribed from M3CEntireVolume::get_number_case0_triangles.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Count triangles for a case-2 cube (two face centers). Transcribed from
// M3CEntireVolume::get_number_case2_triangles.
// -----------------------------------------------------------------------------
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
    std::vector<SiteId> burnt_loop(static_cast<size_t>(numN) + 2, 0);

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

// -----------------------------------------------------------------------------
// Count triangles for a case-M cube (>=3 face centers). Transcribed from
// M3CEntireVolume::get_number_caseM_triangles.
// -----------------------------------------------------------------------------
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
    std::vector<SiteId> burnt_loop(static_cast<size_t>(numN) + 2, 0);

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

// -----------------------------------------------------------------------------
// Count the total triangles across all cubes and set body-center node types.
// Transcribed from M3CEntireVolume::get_number_triangles.
// -----------------------------------------------------------------------------
int64 get_number_triangles(const int32_t* p, Face* sq, const NeighborAccessor& neighbors, int8_t* nodeType, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
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
      int tFCnode = sq[tsq].FCnode;
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
      nodeType[BCnode] = static_cast<int8_t>(nds);
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

// -----------------------------------------------------------------------------
// Generate triangles for a case-0 cube. Transcribed from M3CEntireVolume::get_case0_triangles.
// -----------------------------------------------------------------------------
void get_case0_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, int tin, int* tout, const double tcrd1[3], const double tcrd2[3], SiteId mcid)
{
  const double xhigh = tcrd2[0], yhigh = tcrd2[1], zhigh = tcrd2[2];
  const double xlow = tcrd1[0], ylow = tcrd1[1], zlow = tcrd1[2];

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
  int ctid = tin;
  int where[3];
  double vcrd1[3], vcrd2[3], vcrd3[3];

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
      for(int iii = 0; iii < 3; iii++)
      {
        vcrd1[iii] = v1[tv0].coord[iii];
        vcrd2[iii] = v1[tv1].coord[iii];
        vcrd3[iii] = v1[tv2].coord[iii];
      }
      find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
      for(int jjj = 0; jjj < 3; jjj++)
      {
        t1[ctid].edgePlace[jjj] = where[jjj];
      }
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
      for(int iii = 0; iii < 3; iii++)
      {
        vcrd1[iii] = v1[tv0].coord[iii];
        vcrd2[iii] = v1[tv1].coord[iii];
        vcrd3[iii] = v1[tv2].coord[iii];
      }
      find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
      for(int jjj = 0; jjj < 3; jjj++)
      {
        t1[ctid].edgePlace[jjj] = where[jjj];
      }
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
          for(int iii = 0; iii < 3; iii++)
          {
            vcrd1[iii] = v1[tv0].coord[iii];
            vcrd2[iii] = v1[tv1].coord[iii];
            vcrd3[iii] = v1[tv2].coord[iii];
          }
          find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
          for(int jjj = 0; jjj < 3; jjj++)
          {
            t1[ctid].edgePlace[jjj] = where[jjj];
          }
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
          for(int iii = 0; iii < 3; iii++)
          {
            vcrd1[iii] = v1[tv0].coord[iii];
            vcrd2[iii] = v1[tv1].coord[iii];
            vcrd3[iii] = v1[tv2].coord[iii];
          }
          find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
          for(int jjj = 0; jjj < 3; jjj++)
          {
            t1[ctid].edgePlace[jjj] = where[jjj];
          }
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

// -----------------------------------------------------------------------------
// Generate triangles for a case-2 cube. Transcribed from M3CEntireVolume::get_case2_triangles.
// -----------------------------------------------------------------------------
void get_case2_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, const SiteId* afc, int /*nfctr*/, int tin, int* tout, const double tcrd1[3], const double tcrd2[3],
                         SiteId mcid)
{
  const double xhigh = tcrd2[0], yhigh = tcrd2[1], zhigh = tcrd2[2];
  const double xlow = tcrd1[0], ylow = tcrd1[1], zlow = tcrd1[2];

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
  int ctid = tin;
  int where[3];
  double vcrd1[3], vcrd2[3], vcrd3[3];

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<size_t>(numN) + 2, 0);

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
        for(int iii = 0; iii < 3; iii++)
        {
          vcrd1[iii] = v1[tv0].coord[iii];
          vcrd2[iii] = v1[tv1].coord[iii];
          vcrd3[iii] = v1[tv2].coord[iii];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
        for(int iii = 0; iii < 3; iii++)
        {
          vcrd1[iii] = v1[tv0].coord[iii];
          vcrd2[iii] = v1[tv1].coord[iii];
          vcrd3[iii] = v1[tv2].coord[iii];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
            for(int iii = 0; iii < 3; iii++)
            {
              vcrd1[iii] = v1[tv0].coord[iii];
              vcrd2[iii] = v1[tv1].coord[iii];
              vcrd3[iii] = v1[tv2].coord[iii];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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
            for(int iii = 0; iii < 3; iii++)
            {
              vcrd1[iii] = v1[tv0].coord[iii];
              vcrd2[iii] = v1[tv1].coord[iii];
              vcrd3[iii] = v1[tv2].coord[iii];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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
        for(int iii = 0; iii < 3; iii++)
        {
          vcrd1[iii] = v1[tv0].coord[iii];
          vcrd2[iii] = v1[tv1].coord[iii];
          vcrd3[iii] = v1[tv2].coord[iii];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
        for(int iii = 0; iii < 3; iii++)
        {
          vcrd1[iii] = v1[tv0].coord[iii];
          vcrd2[iii] = v1[tv1].coord[iii];
          vcrd3[iii] = v1[tv2].coord[iii];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
            for(int iii = 0; iii < 3; iii++)
            {
              vcrd1[iii] = v1[tv0].coord[iii];
              vcrd2[iii] = v1[tv1].coord[iii];
              vcrd3[iii] = v1[tv2].coord[iii];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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
            for(int iii = 0; iii < 3; iii++)
            {
              vcrd1[iii] = v1[tv0].coord[iii];
              vcrd2[iii] = v1[tv1].coord[iii];
              vcrd3[iii] = v1[tv2].coord[iii];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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

// -----------------------------------------------------------------------------
// Generate triangles for a case-M cube (fan from body center for open loops).
// Transcribed from M3CEntireVolume::get_caseM_triangles.
// -----------------------------------------------------------------------------
void get_caseM_triangles(Triangle* t1, SiteId* mCubeID, const SiteId* afe, const NodeCoords& v1, Segment* e1, int nfedge, const SiteId* afc, int nfctr, int tin, int* tout, SiteId ccn, const double tcrd1[3],
                         const double tcrd2[3], SiteId mcid)
{
  const double xhigh = tcrd2[0], yhigh = tcrd2[1], zhigh = tcrd2[2];
  const double xlow = tcrd1[0], ylow = tcrd1[1], zlow = tcrd1[2];

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
  int ctid = tin;
  int where[3];
  double vcrd1[3], vcrd2[3], vcrd3[3];

  for(int j1 = 1; j1 < loopID; j1++)
  {
    int openL = 0;
    int flip = 0;
    SiteId startEdge = -1;
    int numN = count[j1];
    to = to + numN;
    from = to - numN;
    std::vector<SiteId> burnt_loop(static_cast<size_t>(numN) + 2, 0);

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
        }
        chaser = burnt_loop[index - 1];
        endNode = static_cast<SiteId>(e1[chaser].node_id[1]);
      } while(index < numN);

      // triangulation: fan from the body-center node ccn
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
        for(int i4 = 0; i4 < 3; i4++)
        {
          vcrd1[i4] = v1[ccn].coord[i4];
          vcrd2[i4] = v1[tn0].coord[i4];
          vcrd3[i4] = v1[tn1].coord[i4];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
        for(int i4 = 0; i4 < 3; i4++)
        {
          vcrd1[i4] = v1[tv0].coord[i4];
          vcrd2[i4] = v1[tv1].coord[i4];
          vcrd3[i4] = v1[tv2].coord[i4];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
        for(int i4 = 0; i4 < 3; i4++)
        {
          vcrd1[i4] = v1[tv0].coord[i4];
          vcrd2[i4] = v1[tv1].coord[i4];
          vcrd3[i4] = v1[tv2].coord[i4];
        }
        find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
        for(int jjj = 0; jjj < 3; jjj++)
        {
          t1[ctid].edgePlace[jjj] = where[jjj];
        }
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
            for(int i4 = 0; i4 < 3; i4++)
            {
              vcrd1[i4] = v1[tv0].coord[i4];
              vcrd2[i4] = v1[tv1].coord[i4];
              vcrd3[i4] = v1[tv2].coord[i4];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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
            for(int i4 = 0; i4 < 3; i4++)
            {
              vcrd1[i4] = v1[tv0].coord[i4];
              vcrd2[i4] = v1[tv1].coord[i4];
              vcrd3[i4] = v1[tv2].coord[i4];
            }
            find_edgePlace(vcrd1, vcrd2, vcrd3, where, xhigh, xlow, yhigh, ylow, zhigh, zlow);
            for(int jjj = 0; jjj < 3; jjj++)
            {
              t1[ctid].edgePlace[jjj] = where[jjj];
            }
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

// -----------------------------------------------------------------------------
// Fill the pre-sized triangle array cube-by-cube. Transcribed from
// M3CEntireVolume::get_triangles.
// -----------------------------------------------------------------------------
void get_triangles(const SiteCoords& p, Triangle* t, SiteId* mCubeID, Face* sq, const NodeCoords& v, Segment* e, SiteId ns, SiteId nsp, int xDim, const std::atomic_bool& shouldCancel)
{
  int tidIn = 0;
  int tidOut = 0;

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
      int tFCnode = sq[tsq].FCnode;
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
// -----------------------------------------------------------------------------
// Match each triangle side to the face edge it lies on (edgePlace 0 = face edge).
// Transcribed from M3CEntireVolume::update_triangle_sides_with_fedge.
// -----------------------------------------------------------------------------
void update_triangle_sides_with_fedge(Triangle* t, const SiteId* mCubeID, const Segment* e, const Face* sq, int64 nT, int xDim, SiteId nsp)
{
  SiteId tFEarray[100];
  int index = 0;
  int prevMCID = -1;

  for(int i = 0; i < nT; i++)
  {
    SiteId ii = mCubeID[i];
    if(ii != prevMCID)
    {
      index = 0;
      SiteId sqID[6];
      sqID[0] = 3 * (ii - 1);
      sqID[1] = 3 * (ii - 1) + 1;
      sqID[2] = 3 * (ii - 1) + 2;
      sqID[3] = 3 * ii + 2;
      sqID[4] = 3 * (ii + xDim - 1) + 1;
      sqID[5] = 3 * (ii + nsp - 1);
      for(int i1 = 0; i1 < 6; i1++)
      {
        int tsq = sqID[i1];
        int nFE = sq[tsq].nEdge;
        for(int i2 = 0; i2 < nFE; i2++)
        {
          tFEarray[index] = sq[tsq].edge_id[i2];
          index++;
        }
      }
    }

    for(int j = 0; j < 3; j++)
    {
      int index1 = j;
      int index2 = (j + 1 == 3) ? 0 : j + 1;
      SiteId cnode1 = t[i].node_id[index1];
      SiteId cnode2 = t[i].node_id[index2];
      for(int k = 0; k < index; k++)
      {
        SiteId cfe = tFEarray[k];
        SiteId tnode1 = static_cast<SiteId>(e[cfe].node_id[0]);
        SiteId tnode2 = static_cast<SiteId>(e[cfe].node_id[1]);
        if((cnode1 == tnode1 && cnode2 == tnode2) || (cnode2 == tnode1 && cnode1 == tnode2))
        {
          t[i].e_id[index1] = cfe;
          t[i].edgePlace[index1] = 0;
          break;
        }
      }
    }
    prevMCID = ii;
  }
}

// -----------------------------------------------------------------------------
// Count unique inner edges (two-pass sizing). Transcribed from
// M3CEntireVolume::get_number_unique_inner_edges.
// -----------------------------------------------------------------------------
int64 get_number_unique_inner_edges(const Triangle* t, const SiteId* mCubeID, int64 nT, const std::atomic_bool& shouldCancel)
{
  // Per marching cube. Sized well above the algorithm's maximum of ~36 unique inner edges per cube.
  SiteId arrayIEnode[120][2];
  int bFlag[120];
  int64 nIE = 0;
  int index = 0;

  for(int mm = 0; mm < 120; mm++)
  {
    bFlag[mm] = 0;
    arrayIEnode[mm][0] = -1;
    arrayIEnode[mm][1] = -1;
  }

  int i = 0;
  do
  {
    if(shouldCancel)
    {
      return 0;
    }
    SiteId cmcID = mCubeID[i];
    SiteId nmcID = (i == (nT - 1)) ? -1 : mCubeID[i + 1];

    for(int j = 0; j < 3; j++)
    {
      if(t[i].edgePlace[j] == 1)
      {
        int index1 = j;
        int index2 = (j + 1 == 3) ? 0 : j + 1;
        arrayIEnode[index][0] = static_cast<SiteId>(t[i].node_id[index1]);
        arrayIEnode[index][1] = static_cast<SiteId>(t[i].node_id[index2]);
        index++;
      }
    }

    if(cmcID != nmcID)
    {
      int nIEDmc = index;
      for(int m = 0; m < nIEDmc; m++)
      {
        bFlag[m] = 0;
      }
      int nIEmc = 0;
      for(int k = 0; k < nIEDmc; k++)
      {
        if(bFlag[k] == 0)
        {
          SiteId cnode1 = arrayIEnode[k][0];
          SiteId cnode2 = arrayIEnode[k][1];
          bFlag[k] = -1;
          for(int kk = 0; kk < nIEDmc; kk++)
          {
            if(bFlag[kk] == 0)
            {
              SiteId nnode1 = arrayIEnode[kk][0];
              SiteId nnode2 = arrayIEnode[kk][1];
              if((cnode1 == nnode1 && cnode2 == nnode2) || (cnode2 == nnode1 && cnode1 == nnode2))
              {
                bFlag[kk] = -1;
              }
            }
          }
          nIEmc++;
        }
      }
      nIE = nIE + nIEmc;
      index = 0;
    }
    i++;
  } while(i < nT);

  return nIE;
}

// -----------------------------------------------------------------------------
// Build unique inner edges with their spins and stamp triangle e_id. Transcribed
// from M3CEntireVolume::get_unique_inner_edges.
// -----------------------------------------------------------------------------
void get_unique_inner_edges(Triangle* t, const SiteId* mCubeID, ISegment* ie, int64 nT, int64 nfedge, const std::atomic_bool& shouldCancel)
{
  // Per marching cube. Sized well above the algorithm's maximum of ~36 unique inner edges per cube.
  int arrayTri[120];
  SiteId arrayIEnode[120][2];
  int bFlag[120];
  int index = 0;
  int64 IEindex = 0;

  for(int mm = 0; mm < 120; mm++)
  {
    bFlag[mm] = 0;
    arrayIEnode[mm][0] = -1;
    arrayIEnode[mm][1] = -1;
  }

  int i = 0;
  do
  {
    if(shouldCancel)
    {
      return;
    }
    SiteId cmcID = mCubeID[i];
    SiteId nmcID = (i == (nT - 1)) ? -1 : mCubeID[i + 1];

    for(int j = 0; j < 3; j++)
    {
      if(t[i].edgePlace[j] == 1)
      {
        int index1 = j;
        int index2 = (j + 1 == 3) ? 0 : j + 1;
        arrayIEnode[index][0] = static_cast<SiteId>(t[i].node_id[index1]);
        arrayIEnode[index][1] = static_cast<SiteId>(t[i].node_id[index2]);
        arrayTri[index] = i;
        index++;
      }
    }

    if(cmcID != nmcID)
    {
      int nIEDmc = index;
      // Only bFlag[0..nIEDmc-1] are ever read below; the legacy loop wrote one past the end.
      for(int m = 0; m < nIEDmc; m++)
      {
        bFlag[m] = 0;
      }

      for(int k = 0; k < nIEDmc; k++)
      {
        if(bFlag[k] == 0)
        {
          SiteId cnode1 = arrayIEnode[k][0];
          SiteId cnode2 = arrayIEnode[k][1];
          bFlag[k] = -1;
          ie[IEindex].node_id[0] = cnode1;
          ie[IEindex].node_id[1] = cnode2;
          int ctri = arrayTri[k];
          ie[IEindex].nSpin[0] = t[ctri].nSpin[0];
          ie[IEindex].nSpin[1] = t[ctri].nSpin[1];
          ie[IEindex].nSpin[2] = 0;
          ie[IEindex].nSpin[3] = 0;
          int tedgeKind = 2;

          for(int jj = 0; jj < 3; jj++)
          {
            if(t[ctri].edgePlace[jj] == 1)
            {
              int index1 = jj;
              int index2 = (jj + 1 == 3) ? 0 : jj + 1;
              SiteId tnode1 = t[ctri].node_id[index1];
              SiteId tnode2 = t[ctri].node_id[index2];
              if((tnode1 == cnode1 && tnode2 == cnode2) || (tnode2 == cnode1 && tnode1 == cnode2))
              {
                t[ctri].e_id[index1] = static_cast<uint64>(IEindex + nfedge);
              }
            }
          }

          for(int kk = 0; kk < nIEDmc; kk++)
          {
            if(bFlag[kk] == 0)
            {
              SiteId nnode1 = arrayIEnode[kk][0];
              SiteId nnode2 = arrayIEnode[kk][1];
              if((cnode1 == nnode1 && cnode2 == nnode2) || (cnode2 == nnode1 && cnode1 == nnode2))
              {
                bFlag[kk] = -1;
                int nspin1Flag = 0;
                int nspin2Flag = 0;
                int ntri = arrayTri[kk];
                int nspin1 = t[ntri].nSpin[0];
                int nspin2 = t[ntri].nSpin[1];
                for(int jjj = 0; jjj < 3; jjj++)
                {
                  if(t[ntri].edgePlace[jjj] == 1)
                  {
                    int index1 = jjj;
                    int index2 = (jjj + 1 == 3) ? 0 : jjj + 1;
                    SiteId tnode1 = t[ntri].node_id[index1];
                    SiteId tnode2 = t[ntri].node_id[index2];
                    if((tnode1 == cnode1 && tnode2 == cnode2) || (tnode2 == cnode1 && tnode1 == cnode2))
                    {
                      t[ntri].e_id[index1] = static_cast<uint64>(IEindex + nfedge);
                    }
                  }
                }
                for(int ii = 0; ii < 4; ii++)
                {
                  if(nspin1 == ie[IEindex].nSpin[ii])
                  {
                    nspin1Flag++;
                  }
                  if(nspin2 == ie[IEindex].nSpin[ii])
                  {
                    nspin2Flag++;
                  }
                }
                if((nspin1Flag * nspin2Flag) == 0)
                {
                  if(nspin1Flag == 0)
                  {
                    ie[IEindex].nSpin[tedgeKind] = nspin1;
                    tedgeKind++;
                  }
                  else if(nspin2Flag == 0)
                  {
                    ie[IEindex].nSpin[tedgeKind] = nspin2;
                    tedgeKind++;
                  }
                  else
                  {
                    ie[IEindex].nSpin[tedgeKind] = nspin1;
                    tedgeKind++;
                    ie[IEindex].nSpin[tedgeKind] = nspin2;
                    tedgeKind++;
                  }
                }
              }
            }
          }
          ie[IEindex].edgeKind = tedgeKind;
          IEindex++;
        }
      }
      index = 0;
    }
    i++;
  } while(i < nT);
}

// -----------------------------------------------------------------------------
// Bump node/edge kinds by 10 on outer-surface triangles (label sign flip).
// Transcribed from M3CEntireVolume::update_node_edge_kind.
// -----------------------------------------------------------------------------
void update_node_edge_kind(int8_t* nodeType, Segment* fe, ISegment* ie, const Triangle* t, int64 nT, int64 nfedge)
{
  for(int j = 0; j < nT; j++)
  {
    int tspin1 = t[j].nSpin[0];
    int tspin2 = t[j].nSpin[1];
    if(tspin1 * tspin2 < 0)
    {
      for(int i = 0; i < 3; i++)
      {
        SiteId tn = t[j].node_id[i];
        int tnkind = nodeType[tn];
        if(tnkind < 10)
        {
          nodeType[tn] = static_cast<int8_t>(tnkind + 10);
        }
        SiteId te = t[j].e_id[i];
        if(te < nfedge)
        {
          if(fe[te].edgeKind < 10)
          {
            fe[te].edgeKind = fe[te].edgeKind + 10;
          }
        }
        else
        {
          te = te - nfedge;
          if(ie[te].edgeKind < 10)
          {
            ie[te].edgeKind = ie[te].edgeKind + 10;
          }
        }
      }
    }
  }
}

// Convert a 1-based padded working-grid site to the 0-based index into the original (unpadded)
// cell arrays, or SIZE_MAX if the site is in the ghost shell.
usize paddedSiteToOriginalCell(int64 site, const size_t fileDim[3], const size_t dims[3])
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

// Find a representative original cell for a working label among the 8 corner sites of a marching
// cube. Returns SIZE_MAX if no non-ghost corner carries that label (i.e. the label is exterior).
usize findSourceCell(int workLabel, int64 cubeSite, const NeighborAccessor& n, const int32_t* point, const size_t fileDim[3], const size_t dims[3])
{
  const Neighbor nb = n[cubeSite]; // cache: 7 neighbors of the cube site read below
  const int64 cornerSites[8] = {cubeSite,
                                nb.neigh_id[1],
                                nb.neigh_id[7],
                                nb.neigh_id[8],
                                nb.neigh_id[18],
                                nb.neigh_id[19],
                                nb.neigh_id[25],
                                nb.neigh_id[26]};
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

// -----------------------------------------------------------------------------
// Shared finalization for both the whole-volume and sliding-window variants: given the assembled
// triangles/mCubeID/fedges and the (fully populated) nodeType array, build inner-edge connectivity,
// refine node kinds, compact node ids, and write the output TriangleGeom + FaceLabels + NodeTypes,
// then transfer selected arrays and (optionally) repair triangle windings. Kept as a free function so
// it can name the file-local mesh types. Callers must have already run the triangle-side/face-edge
// matching (update_triangle_sides_with_fedge, done per-cube in the windowed sweep or as a whole-volume
// pass in runEntireVolume).
Result<> finalizeMesh(DataStructure& dataStructure, const M3CSurfaceMeshingInputValues* inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                      std::vector<Triangle>& triangles, std::vector<SiteId>& mCubeID, std::vector<Segment>& fedges, std::vector<int8_t>& nodeType, std::vector<int32_t>& point,
                      const NodeCoords& nodeCoords, const NeighborAccessor& neighbors, SiteId NS, const size_t* fileDim, const size_t* dims, int maxGrainId)
{
  const int64 nTriangle = static_cast<int64>(triangles.size());
  const int64 nFEdge = static_cast<int64>(fedges.size());

  const int64 nIEdge = get_number_unique_inner_edges(triangles.data(), mCubeID.data(), nTriangle, shouldCancel);
  std::vector<ISegment> iedges(static_cast<size_t>(nIEdge < 0 ? 0 : nIEdge));
  get_unique_inner_edges(triangles.data(), mCubeID.data(), iedges.data(), nTriangle, nFEdge, shouldCancel);

  update_node_edge_kind(nodeType.data(), fedges.data(), iedges.data(), triangles.data(), nTriangle, nFEdge);

  // Face/inner edge segments are no longer needed after node-kind refinement; release them before the
  // memory-heavy output write + winding-repair stages.
  std::vector<Segment>().swap(fedges);
  std::vector<ISegment>().swap(iedges);

  if(shouldCancel)
  {
    return {};
  }

  // --- Stage 5: compact nodes and write the output TriangleGeom --------------
  messageHandler("Writing surface mesh...");
  // Node-id compaction without a dense 7*NS candidate->id map. A candidate's compacted id is simply
  // the number of real nodes (nodeType > 0) that precede it; we answer that from a coarse per-block
  // prefix over nodeType plus a small in-block scan (saves ~3.8 GB at 512^3 vs a uint32 map). This is
  // valid because update_node_edge_kind above only adds +10 to kinds and never clears a node, so the
  // set of real nodes is exactly what the sweep produced.
  const SiteId numCandidateNodes = 7 * NS;
  constexpr SiteId k_NodeBlock = 128;
  const SiteId numNodeBlocks = (numCandidateNodes + k_NodeBlock - 1) / k_NodeBlock;
  std::vector<uint32_t> nodeBlockBase(static_cast<size_t>(numNodeBlocks));
  int64 realNodeRunning = 0;
  for(SiteId b = 0; b < numNodeBlocks; b++)
  {
    nodeBlockBase[static_cast<size_t>(b)] = static_cast<uint32_t>(realNodeRunning);
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
    int64 r = nodeBlockBase[static_cast<size_t>(c / k_NodeBlock)];
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
  triangleGeom.resizeFaceList(static_cast<usize>(nTriangle));
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nNodes)});
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({static_cast<usize>(nTriangle)});

  auto& vertexStore = triangleGeom.getVertices()->getDataStoreRef();
  auto& triStore = triangleGeom.getFaces()->getDataStoreRef();
  auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(inputValues->FaceLabelsDataPath).getDataStoreRef();
  auto& nodeTypesOut = dataStructure.getDataRefAs<Int8Array>(inputValues->NodeTypesDataPath).getDataStoreRef();
  faceLabels.resizeTuples({static_cast<usize>(nTriangle)});
  nodeTypesOut.resizeTuples({static_cast<usize>(nNodes)});

  // Compact nodes: scatter coordinates + node types to their new (sequential) ids. Walking candidates
  // in ascending order and emitting with a running counter reproduces assign_new_nodeID's numbering.
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

  // FaceLabels convention (matches QuickSurfaceMesh / SurfaceNets): negative ghost labels -> -1
  // (exterior box surface), the renumbered zero-feature (maxGrainId) -> 0, and the SMALLER of the two
  // labels is placed in component 0 (downstream filters rely on this). Triangle winding is made
  // consistent with this ordering separately by the optional repair pass below.
  const auto toFaceLabel = [maxGrainId](int nSpin) -> int32 { return (nSpin < 0) ? -1 : ((nSpin == maxGrainId) ? 0 : nSpin); };

  // Triangles: remap to compacted node ids and write the ordered FaceLabels.
  for(int64 i = 0; i < nTriangle; i++)
  {
    triStore[static_cast<usize>(i) * 3 + 0] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[0]));
    triStore[static_cast<usize>(i) * 3 + 1] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[1]));
    triStore[static_cast<usize>(i) * 3 + 2] = static_cast<IGeometry::MeshIndexType>(compactedNodeId(triangles[i].node_id[2]));

    const int32 labelA = toFaceLabel(triangles[i].nSpin[0]);
    const int32 labelB = toFaceLabel(triangles[i].nSpin[1]);
    faceLabels[static_cast<usize>(i) * 2 + 0] = (labelA <= labelB) ? labelA : labelB;
    faceLabels[static_cast<usize>(i) * 2 + 1] = (labelA <= labelB) ? labelB : labelA;
  }

  // --- Transfer selected Cell/Feature arrays to the two sides of each face -----------------------
  // Reuses the simplnx TupleTransfer machinery. For each triangle, a representative source cell is
  // derived per side from the triangle's marching-cube corner cells (matched by working label). The
  // exterior side (FaceLabel == -1) is skipped by the transfer functions.
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

    for(int64 i = 0; i < nTriangle; i++)
    {
      // Match the smaller-first FaceLabel ordering above so each transferred component aligns with
      // the feature in the same FaceLabels component.
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

  // The M3C working buffers are dead once the output mesh and attribute transfer are written; the
  // winding-repair pass below reads only the output TriangleGeom + FaceLabels. Release them here so
  // findElementNeighbors' adjacency is not allocated on top of them (this is where peak memory lands).
  std::vector<Triangle>().swap(triangles);
  std::vector<SiteId>().swap(mCubeID);
  std::vector<int8_t>().swap(nodeType);
  std::vector<int32_t>().swap(point);
  std::vector<uint32_t>().swap(nodeBlockBase);

  // Optional winding-consistency repair. M3C does not itself guarantee globally consistent normals,
  // so this pass (using triangle connectivity) makes the winding consistent with the FaceLabels.
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

  return {};
}
} // namespace

namespace nx::core
{
// -----------------------------------------------------------------------------
M3CSurfaceMeshing::M3CSurfaceMeshing(DataStructure& dataStructure, M3CSurfaceMeshingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
M3CSurfaceMeshing::~M3CSurfaceMeshing() noexcept = default;

// -----------------------------------------------------------------------------
Result<> M3CSurfaceMeshing::operator()()
{
  // Dev toggle: the sliding-window variant is being brought up alongside the proven whole-volume
  // variant. It must produce byte-identical output; once verified it will become the default.
  // if(const char* windowed = std::getenv("M3C_WINDOWED"); windowed != nullptr && std::string_view(windowed) == "1")
  // {
    return runWindowed();
  // }
  // return runEntireVolume();
}

// -----------------------------------------------------------------------------
Result<> M3CSurfaceMeshing::runEntireVolume()
{
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();

  // Dimensions (cell counts). Spacing/origin from ImageGeom; RectGrid falls back to unit spacing (TODO).
  SizeVec3 gridDims = gridGeom.getDimensions();
  size_t dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  float res[3] = {1.0f, 1.0f, 1.0f};
  float origin[3] = {0.0f, 0.0f, 0.0f};
  if(const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->GridGeomDataPath); imageGeom != nullptr)
  {
    FloatVec3 spacing = imageGeom->getSpacing();
    FloatVec3 imgOrigin = imageGeom->getOrigin();
    res[0] = spacing[0];
    res[1] = spacing[1];
    res[2] = spacing[2];
    origin[0] = imgOrigin[0];
    origin[1] = imgOrigin[1];
    origin[2] = imgOrigin[2];
  }

  // Always wrap the volume in a ghost layer (NX inputs are not pre-wrapped).
  constexpr bool k_AddSurfaceLayer = true;
  size_t fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  const size_t totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  // Site count and per-plane site count are 64-bit (SiteId): they index the FeatureId grid and must
  // support volumes with more than 2^31 voxels. (Edge/node id storage is unsigned 32-bit, capping the
  // MESH size near 2^32 elements; the peak memory model still scales with volume - see docs.)
  const SiteId NS = static_cast<SiteId>(totalPoints);
  const SiteId NSP = static_cast<SiteId>(fileDim[0] * fileDim[1]);

  // Read FeatureIds directly from its DataStore (no full-array copy) into the padded working grid.
  // The FeatureId==0 renumbering happens on the working grid, never on the input array.
  m_MessageHandler("Initializing working grid and ghost layer...");
  std::vector<int32_t> point(totalPoints + 1, 0);
  const int maxGrainId = initialize_micro(k_AddSurfaceLayer, dims, fileDim, featureIdsStore, point.data());

  // On-demand accessors replace the former full-volume voxCoords, node, and neighbor arrays.
  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {res[0], res[1], res[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const NeighborAccessor neighbors{NS, NSP, static_cast<int>(fileDim[0])};

  // 3 marching squares (top/back/left) per site; node types (7 candidate nodes/site) start Unused (0).
  m_MessageHandler("Initializing candidate nodes and squares...");
  std::vector<Face> squares(static_cast<size_t>(3) * NS);
  std::vector<int8_t> nodeType(static_cast<size_t>(7) * NS, 0);
  initialize_squares(squares.data(), NS);

  if(m_ShouldCancel)
  {
    return {};
  }

  // --- Stage 2: face edges ---------------------------------------------------
  m_MessageHandler("Counting face edges...");
  const int64 nFEdge = get_number_fEdges(squares.data(), point.data(), neighbors, NS, m_ShouldCancel);

  m_MessageHandler("Finding nodes and edges on each square...");
  std::vector<Segment> fedges(static_cast<size_t>(nFEdge < 0 ? 0 : nFEdge));
  get_nodes_fEdges(squares.data(), point.data(), neighbors, nodeType.data(), fedges.data(), NS, NSP, static_cast<int>(fileDim[0]), m_ShouldCancel);

  if(m_ShouldCancel)
  {
    return {};
  }

  // --- Stage 3: triangles ----------------------------------------------------
  m_MessageHandler("Counting triangles...");
  const int64 nTriangle = get_number_triangles(point.data(), squares.data(), neighbors, nodeType.data(), fedges.data(), NS, NSP, static_cast<int>(fileDim[0]), m_ShouldCancel);

  m_MessageHandler("Generating triangles...");
  std::vector<Triangle> triangles(static_cast<size_t>(nTriangle < 0 ? 0 : nTriangle));
  std::vector<SiteId> mCubeID(static_cast<size_t>(nTriangle < 0 ? 0 : nTriangle), 0);
  get_triangles(siteCoords, triangles.data(), mCubeID.data(), squares.data(), nodeCoords, fedges.data(), NS, NSP, static_cast<int>(fileDim[0]), m_ShouldCancel);

  if(m_ShouldCancel)
  {
    return {};
  }

  // --- Stage 4: triangle-side/edge connectivity, inner edges, winding --------
  m_MessageHandler("Building triangle/edge connectivity...");
  update_triangle_sides_with_fedge(triangles.data(), mCubeID.data(), fedges.data(), squares.data(), nTriangle, static_cast<int>(fileDim[0]), NSP);

  return finalizeMesh(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel, triangles, mCubeID, fedges, nodeType, point,
                       nodeCoords, neighbors, NS, fileDim, dims, maxGrainId);
}

// -----------------------------------------------------------------------------
// Sliding-window (z-slice) variant. See M3CSurfaceMeshing.hpp and the design notes: fuses the
// whole-volume passes into a single z-marching sweep so per-site scratch stays O(sliceArea). Being
// implemented incrementally; must remain byte-identical to runEntireVolume().
Result<> M3CSurfaceMeshing::runWindowed()
{
  // --- Setup (identical to runEntireVolume) ----------------------------------
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();

  SizeVec3 gridDims = gridGeom.getDimensions();
  size_t dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  float res[3] = {1.0f, 1.0f, 1.0f};
  float origin[3] = {0.0f, 0.0f, 0.0f};
  if(const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->GridGeomDataPath); imageGeom != nullptr)
  {
    FloatVec3 spacing = imageGeom->getSpacing();
    FloatVec3 imgOrigin = imageGeom->getOrigin();
    res[0] = spacing[0];
    res[1] = spacing[1];
    res[2] = spacing[2];
    origin[0] = imgOrigin[0];
    origin[1] = imgOrigin[1];
    origin[2] = imgOrigin[2];
  }

  constexpr bool k_AddSurfaceLayer = true;
  size_t fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  const size_t totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  const SiteId NS = static_cast<SiteId>(totalPoints);
  const SiteId NSP = static_cast<SiteId>(fileDim[0] * fileDim[1]);
  const int xDim = static_cast<int>(fileDim[0]);

  m_MessageHandler("Initializing working grid and ghost layer...");
  std::vector<int32_t> point(totalPoints + 1, 0);
  const int maxGrainId = initialize_micro(k_AddSurfaceLayer, dims, fileDim, featureIdsStore, point.data());

  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {res[0], res[1], res[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const NeighborAccessor neighbors{NS, NSP, xDim};

  // Mesh-scale accumulators grow with the output; only the per-site squares are windowed.
  std::vector<int8_t> nodeType(static_cast<size_t>(7) * NS, 0);
  std::vector<Segment> fedges;
  std::vector<Triangle> triangles;
  std::vector<SiteId> mCubeID;

  // --- Square window: two z-slices of the 3-marching-squares-per-site scratch ------------------
  // squareId = 3*(site-1)+ord. The window holds squares for sites [winBaseSite, winBaseSite + 2*NSP);
  // winIndex maps an absolute squareId to its slot. As the cube sweep crosses into the next slice the
  // window slides forward by one slice (memmove the second slice down, compute the new second slice).
  const SiteId sliceSquares = 3 * NSP; // squares per z-slice
  std::vector<Face> window(static_cast<size_t>(2) * sliceSquares);
  SiteId winBaseSite = 1;
  auto winIndex = [&winBaseSite](SiteId squareId) -> size_t { return static_cast<size_t>(squareId - 3 * (winBaseSite - 1)); };

  // Compute the edges/flags for a contiguous range of squareIds directly into the window buffer.
  // This is the get_nodes_fEdges body restricted to one slice; it appends face edges to the global
  // fedges (edge ids stay globally sequential because squareIds are visited in ascending order) and
  // sets face-center/edge node types. It also reproduces get_number_fEdges's per-square effect flag.
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
      int atBulk = 0;
      for(int m = 0; m < 4; m++)
      {
        tnspin[m] = point[tnsite[m]];
        if(tnspin[m] < 0)
        {
          atBulk++;
        }
      }
      if(atBulk != 4)
      {
        sqk.effect = 1;
      }

      int edgeCount = 0;
      if(atBulk != 4)
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
              get_nodes(cubeOrigin, sqOrder, nodeIndex, nodeID, NSP, xDim);
              get_spins(point.data(), cubeOrigin, sqOrder, pixIndex, pixSpin, NSP, xDim);

              if(pixSpin[0] > 0 || pixSpin[1] > 0)
              {
                Segment seg;
                seg.node_id[0] = nodeID[0];
                seg.node_id[1] = nodeID[1];
                seg.nSpin[0] = pixSpin[0];
                seg.nSpin[1] = pixSpin[1];
                seg.edgeKind = 2;
                sqk.edge_id[edgeCount] = static_cast<uint32_t>(eid);
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
                  SiteId tnode = nodeID[ii];
                  if(nodeType[tnode] != -1)
                  {
                    nodeType[tnode] = M3CNodeType::k_Default;
                  }
                }
              }
            }
          }
        }
      }
      sqk.nEdge = edgeCount;
    }
  };

  // The case-table functions (get_number_caseX / get_caseX) FLIP shared face edges in place while
  // tracing loops, so the whole-volume ordering (ALL counts, then ALL generates) is significant for a
  // byte-identical result. We reproduce it with two windowed sweeps over the same persistent fedges:
  //   pass 1 (appendEdges=true,  generate=false): build fedges, set node types, and run every count
  //                                               (applying all count-flips in cube order);
  //   pass 2 (appendEdges=false, generate=true):  rebuild the windowed squares (no fedges append) and
  //                                               generate triangles (applying all generate-flips).
  int64 nTriangle = 0;

  auto sweep = [&](bool appendEdges, bool generate) {
    winBaseSite = 1;
    int64 eid = 0;
    int64 tidRun = 0;
    computeSquares(0, std::min<SiteId>(2 * sliceSquares, 3 * NS), appendEdges, eid);

    for(SiteId i = 1; i <= (NS - NSP); i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Slide the window forward so it always covers sites [i, i + NSP].
      while(i >= winBaseSite + NSP)
      {
        std::memmove(window.data(), window.data() + sliceSquares, static_cast<size_t>(sliceSquares) * sizeof(Face));
        winBaseSite += NSP;
        const SiteId newLoSquare = 3 * (winBaseSite + NSP - 1);
        const SiteId newHiSquare = std::min<SiteId>(3 * (winBaseSite + 2 * NSP - 1), 3 * NS);
        if(newLoSquare < newHiSquare)
        {
          computeSquares(newLoSquare, newHiSquare, appendEdges, eid);
        }
      }

      SiteId sqID[6];
      sqID[0] = 3 * (i - 1);
      sqID[1] = 3 * (i - 1) + 1;
      sqID[2] = 3 * (i - 1) + 2;
      sqID[3] = 3 * i + 2;
      sqID[4] = 3 * (i + xDim - 1) + 1;
      sqID[5] = 3 * (i + NSP - 1);

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

      // Body-center node type (get_number_triangles nFC>=3 block; point-grid only). Set in the count
      // pass, matching where the whole-volume path assigns it.
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
        nodeType[BCnode] = static_cast<int8_t>(nds);
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
        // Pass 1: count triangles (and apply the same shared-edge flips as the whole-volume count).
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

      // Pass 2: generate triangles into the pre-sized global arrays (running index like get_triangles).
      double coord1[3];
      double coord2[3];
      for(int k = 0; k < 3; k++)
      {
        coord1[k] = siteCoords[i].coord[k];
        coord2[k] = siteCoords[i + 1 + xDim + NSP].coord[k];
      }
      int tin = static_cast<int>(tidRun);
      int tout = tin;
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

      // Match each new triangle's sides to the cube's face edges (per-cube update_triangle_sides).
      SiteId tFEarray[100];
      int idx = 0;
      for(int i1 = 0; i1 < 6; i1++)
      {
        const Face& sqf = window[winIndex(sqID[i1])];
        int nfe = sqf.nEdge;
        for(int i2 = 0; i2 < nfe; i2++)
        {
          tFEarray[idx] = sqf.edge_id[i2];
          idx++;
        }
      }
      for(int t = tin; t < tout; t++)
      {
        for(int j = 0; j < 3; j++)
        {
          int index1 = j;
          int index2 = (j + 1 == 3) ? 0 : j + 1;
          SiteId cnode1 = triangles[t].node_id[index1];
          SiteId cnode2 = triangles[t].node_id[index2];
          for(int k = 0; k < idx; k++)
          {
            SiteId cfe = tFEarray[k];
            SiteId tnode1 = static_cast<SiteId>(fedges[cfe].node_id[0]);
            SiteId tnode2 = static_cast<SiteId>(fedges[cfe].node_id[1]);
            if((cnode1 == tnode1 && cnode2 == tnode2) || (cnode2 == tnode1 && cnode1 == tnode2))
            {
              triangles[t].e_id[index1] = cfe;
              triangles[t].edgePlace[index1] = 0;
              break;
            }
          }
        }
      }
    }
  };

  m_MessageHandler("Sweeping z-slices (pass 1: face edges + triangle count)...");
  sweep(true, false);
  if(m_ShouldCancel)
  {
    return {};
  }
  triangles.resize(static_cast<size_t>(nTriangle < 0 ? 0 : nTriangle));
  mCubeID.resize(static_cast<size_t>(nTriangle < 0 ? 0 : nTriangle), 0);

  m_MessageHandler("Sweeping z-slices (pass 2: generating triangles)...");
  sweep(false, true);

  if(m_ShouldCancel)
  {
    return {};
  }

  return finalizeMesh(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel, triangles, mCubeID, fedges, nodeType, point, nodeCoords, neighbors, NS, fileDim, dims, maxGrainId);
}
} // namespace nx::core
