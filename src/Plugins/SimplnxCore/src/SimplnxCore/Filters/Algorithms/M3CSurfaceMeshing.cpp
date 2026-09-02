#include "M3CSurfaceMeshing.hpp"

#include "SimplnxCore/Filters/Algorithms/TupleTransfer.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
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
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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
// Index of a padded site / voxel (i.e. an index into the FeatureId grid). MUST be 64-bit: a large
// Image Geometry can have well over 2^31 voxels, and node ids derived as 7*site must not overflow.
using SiteId = int64;
// Sentinel stored in the compacted-node map for candidate slots that are not real mesh nodes.
constexpr uint32 k_UnusedNodeId = std::numeric_limits<uint32>::max();

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
  uint32 edge_id[4];
  SiteId FCnode; // face-center node id, -1 if none
  int8 nEdge;
  int8 effect; // 0 = useless square, 1 = straddles >=2 labels
};
struct Segment // a face edge
{
  int64 node_id[2];
  int nSpin[2]; // labels on left/right of the arrow
};
struct Triangle
{
  int64 node_id[3];
  int nSpin[2];
};

// On-demand coordinate accessors. A site's coordinate and each of its 7 candidate node positions are
// pure functions of the 1-based padded site index, so they are computed as needed rather than stored
// in full-volume arrays (which dominated the algorithm's memory footprint).
struct SiteCoords
{
  usize fileDim0;
  usize fileDim1;
  usize fileNSP; // fileDim0 * fileDim1
  float res[3];
  float origin[3];

  VoxelCoord operator[](int64 site) const
  {
    const usize linear = static_cast<usize>(site - 1);
    // Subtract the 1-cell ghost shell so that real cell (0,0,0) at padded index (1,1,1) maps to the
    // geometry origin, keeping M3C's coordinates aligned with the input volume and the other meshers.
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

  // Independent grain id for the (formerly) zero feature
  maxGrainId = maxGrainId + 1;

  p[0] = 0; // Point 0 is garbage

  // Renumber the (formerly) zero feature to the reserved id. Ghost cells are negative and untouched.
  // Voxel coordinates are no longer stored; they are computed on demand via SiteCoords/NodeCoords.
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
      sq[sqId].edge_id[j] = k_UnusedNodeId;
    }
    sq[sqId].nEdge = 0;
    sq[sqId].FCnode = -1;
    sq[sqId].effect = 0;
  }
}

// Node type values (match SIMPL::SurfaceMesh::NodeType / what simplnx LaplacianSmoothing consumes).
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

// -----------------------------------------------------------------------------
// Map a square's two side-pixel slots to the two straddling FeatureIds.
// Transcribed from M3CEntireVolume::get_spins.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Count the total number of face edges across all squares (two-pass sizing).
// Transcribed from M3CEntireVolume::get_number_fEdges.
// -----------------------------------------------------------------------------
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
                // Unconditional: an edge-endpoint node touched by any square edge is a real mesh node.
                // (The legacy `if(nodeKind != -1)` guard was vestigial -- legacy also zero-initializes
                // node kinds, so the value -1 never occurs and the branch was always taken. A node
                // previously marked k_Unused by a box-surface edge MUST be promoted here, otherwise
                // node compaction would drop a node that stored edges/triangles still reference.)
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
//
// Chase-loop guard note (applies to all 8 burnt_loop chase loops in the case2/caseM count and
// generate functions): for well-formed label data every do-pass extends the chain by EXACTLY one
// edge -- endNode is fixed for the whole pass, the cube's face edges contain no duplicate node
// pairs (adjacent cube faces share only one edge-midpoint node and the 2D case tables never emit
// duplicate edges), and within one spin-pair loop every node has degree <= 2 (the 3-4 edges that
// meet at a face-center node all carry distinct spin pairs). So the loop of numN edges closes in
// exactly numN passes and index never exceeds numN. The in-loop `index >= numN` break and the
// per-pass no-progress break are therefore unreachable for valid input; they exist so that
// degenerate/non-manifold input degrades to a bounded (possibly incomplete) loop instead of
// overrunning burnt_loop or spinning forever.
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

// -----------------------------------------------------------------------------
// Count the total triangles across all cubes and set body-center node types.
// Transcribed from M3CEntireVolume::get_number_triangles.
// -----------------------------------------------------------------------------
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
      // 5-8 distinct labels can legitimately meet at a cube's body center, but the published NodeType
      // convention tops out at k_QuadPoint (= "4 or more grains"; downstream consumers such as
      // Laplacian smoothing only handle 2/3/4 and their +10 surface variants). Clamp like
      // QuickSurfaceMesh does. (Legacy wrote the raw count.)
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

      // Case dispatch (this exhausts all reachable values of nFC -- the same reasoning applies to the
      // matching dispatches in get_triangles and the windowed/parallel sweeps): a face contributes a
      // face-center node only for square cases 7/11/13/14/19, which is purely a function of the 4
      // corner labels. Exhaustively enumerating all 4140 label partitions of a cube's 8 corners yields
      // nFC in {0, 2, 3, 4, 5, 6} -- exactly one face-center is geometrically impossible (a lone
      // triple-line crossing cannot terminate inside the cube without turning on other faces), so the
      // nFC == 1 gap here silently skips nothing. nFC > 6 cannot occur (a cube has 6 faces).
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

// -----------------------------------------------------------------------------
// Generate triangles for a case-2 cube. Transcribed from M3CEntireVolume::get_case2_triangles.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Generate triangles for a case-M cube (fan from body center for open loops).
// Transcribed from M3CEntireVolume::get_caseM_triangles.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Fill the pre-sized triangle array cube-by-cube. Transcribed from
// M3CEntireVolume::get_triangles.
// -----------------------------------------------------------------------------
// Sharp Bounding Box Edges support.
//
// M3C's candidate nodes sit on a half-cell lattice: an edge-midpoint node has cell-centre coordinates on
// two axes and a cell-face coordinate on the third, a face-centre node has one cell-centre coordinate and
// a body centre none. Along a bounding-box edge the marching square straddling it has one real corner and
// three ghost corners, and the case table joins its two edge midpoints with a diagonal: a 45 degree
// chamfer half a cell deep on both walls. The chamfer vertices are exactly the OUTERMOST row of wall
// nodes, because on a wall the only nodes within half a cell of a neighbouring wall are those whose
// cell-centre coordinate lies in the first or last cell along that axis. Snapping that row onto the
// neighbouring wall plane extends both walls to the edge line, where the two rows coincide and are
// merged; the chamfer triangles then reference a repeated node and are dropped.
//
// Everything is decided on the integer half-cell lattice, never on float coordinates, so the pass is
// exact and independent of spacing and origin.
struct HalfCellLattice
{
  const NodeCoords& nodeCoords;

  // Position of candidate node `id` in half-cell units from the volume origin, i.e. the node's coordinate
  // is origin + u * spacing / 2. The bounding planes are u == 0 and u == 2 * dims; the outermost rows of
  // cell-centre nodes are u == 1 and u == 2 * dims - 1.
  std::array<int64, 3> operator()(SiteId id) const
  {
    const SiteCoords& sites = nodeCoords.sites;
    const usize linear = static_cast<usize>(id / 7);
    const int kind = static_cast<int>(id % 7);
    // Same padded-index decomposition as SiteCoords::operator[] (real cell (0,0,0) is padded (1,1,1)).
    const int64 i = static_cast<int64>(linear % sites.fileDim0) - 1;
    const int64 j = static_cast<int64>((linear / sites.fileDim0) % sites.fileDim1) - 1;
    const int64 k = static_cast<int64>(linear / sites.fileNSP) - 1;
    // Which axes carry the +half-spacing offset for this node kind (see NodeCoords::operator[]).
    const bool offX = (kind == 0 || kind == 3 || kind == 4 || kind == 6);
    const bool offY = (kind == 1 || kind == 3 || kind == 5 || kind == 6);
    const bool offZ = (kind == 2 || kind == 4 || kind == 5 || kind == 6);
    return {2 * i + 1 + (offX ? 1 : 0), 2 * j + 1 + (offY ? 1 : 0), 2 * k + 1 + (offZ ? 1 : 0)};
  }
};

// Result of the sharp-edge pass: coordinate overrides for the nodes it moved (every other node keeps
// nodeCoords[id]) and the number of chamfer triangles it removed.
struct SharpEdgeResult
{
  std::unordered_map<SiteId, Node> SnappedCoords;
  int64 NumFacesRemoved = 0;
};

// Applies the Sharp Bounding Box Edges pass described above to the surviving triangles. Must run after
// the exterior (+10) NodeType promotion, which is how boundary nodes are recognised, and before node
// compaction, since it clears the NodeType of merged-away nodes and drops triangles in place.
SharpEdgeResult sharpenBoundingBoxEdges(std::vector<Triangle>& triangles, std::vector<SiteId>& mCubeID, std::vector<int8>& nodeType, SiteId numCandidateNodes, const NodeCoords& nodeCoords,
                                        const usize dims[3])
{
  SharpEdgeResult result;
  const HalfCellLattice lattice{nodeCoords};
  const SiteCoords& sites = nodeCoords.sites;
  const std::array<int64, 3> wallHi = {2 * static_cast<int64>(dims[0]), 2 * static_cast<int64>(dims[1]), 2 * static_cast<int64>(dims[2])};
  // Lattice positions packed into one integer for hashing.
  const auto packLattice = [&wallHi](const std::array<int64, 3>& u) -> uint64 { return static_cast<uint64>((u[2] * (wallHi[1] + 1) + u[1]) * (wallHi[0] + 1) + u[0]); };

  // Pass 1: for every boundary node decide its snapped lattice position; nodes landing on the same
  // position are merged into the first (lowest id) one to get there, which keeps the pass deterministic.
  std::unordered_map<uint64, SiteId> representativeByPosition;
  std::unordered_map<SiteId, SiteId> mergedInto;
  for(SiteId id = 0; id < numCandidateNodes; id++)
  {
    if(nodeType[static_cast<usize>(id)] < 10)
    {
      continue; // interior node, or unused candidate
    }
    std::array<int64, 3> u = lattice(id);
    bool onWall = false;
    for(usize ax = 0; ax < 3; ax++)
    {
      onWall = onWall || u[ax] == 0 || u[ax] == wallHi[ax];
    }
    if(!onWall)
    {
      continue; // cannot happen for a promoted node; guards the lattice arithmetic
    }
    std::array<bool, 3> snappedAxis = {false, false, false};
    for(usize ax = 0; ax < 3; ax++)
    {
      // A one-cell-thick axis has a single cell-centre row that is half a cell from BOTH of its bounding
      // planes; there is no unambiguous edge to snap it to, so that axis is left chamfered.
      if(dims[ax] < 2)
      {
        continue;
      }
      if(u[ax] == 1)
      {
        u[ax] = 0;
        snappedAxis[ax] = true;
      }
      else if(u[ax] == wallHi[ax] - 1)
      {
        u[ax] = wallHi[ax];
        snappedAxis[ax] = true;
      }
    }
    const auto [it, inserted] = representativeByPosition.try_emplace(packLattice(u), id);
    if(inserted)
    {
      if(snappedAxis[0] || snappedAxis[1] || snappedAxis[2])
      {
        // Keep the node's own float coordinates on the axes that did not move, and put it EXACTLY on the
        // plane value the rest of simplnx derives for the volume bounds on the axes that did.
        Node node = nodeCoords[id];
        for(usize ax = 0; ax < 3; ax++)
        {
          if(snappedAxis[ax])
          {
            node.coord[ax] = (u[ax] == 0) ? sites.origin[ax] : sites.origin[ax] + static_cast<float>(dims[ax]) * sites.res[ax];
          }
        }
        result.SnappedCoords.emplace(id, node);
      }
    }
    else
    {
      const SiteId representative = it->second;
      mergedInto.emplace(id, representative);
      nodeType[static_cast<usize>(representative)] = std::max(nodeType[static_cast<usize>(representative)], nodeType[static_cast<usize>(id)]);
      nodeType[static_cast<usize>(id)] = M3CNodeType::k_Unused;
    }
  }

  if(mergedInto.empty())
  {
    return result;
  }

  // Every node the pass touched (moved or merged into). Used to find the degenerate triangles left on the
  // edge lines, and afterwards to clear any of these nodes no surviving triangle references.
  std::unordered_set<SiteId> touched;
  for(const auto& [id, node] : result.SnappedCoords)
  {
    touched.insert(id);
  }
  for(const auto& [id, representative] : mergedInto)
  {
    touched.insert(representative);
  }
  const auto finalCoord = [&result, &nodeCoords](SiteId id) -> Node {
    const auto it = result.SnappedCoords.find(id);
    return (it != result.SnappedCoords.end()) ? it->second : nodeCoords[id];
  };

  // Pass 2: remap the triangles' node ids and drop the ones the merge collapsed. A chamfer triangle has
  // two vertices on the same cell of the edge line, so after the merge it repeats a node id. The only
  // other way a triangle can lose its area here is for all three vertices to end up on one edge line
  // (exactly collinear, so the cross product is exactly zero); that is checked only for triangles made
  // entirely of touched nodes, which is the sole place it can arise.
  const int64 nTriangle = static_cast<int64>(triangles.size());
  int64 survivingCount = 0;
  for(int64 i = 0; i < nTriangle; i++)
  {
    Triangle triangle = triangles[static_cast<usize>(i)];
    int numTouched = 0;
    for(int corner = 0; corner < 3; corner++)
    {
      const auto it = mergedInto.find(triangle.node_id[corner]);
      if(it != mergedInto.end())
      {
        triangle.node_id[corner] = it->second;
      }
      if(touched.count(triangle.node_id[corner]) != 0)
      {
        numTouched++;
      }
    }
    if(triangle.node_id[0] == triangle.node_id[1] || triangle.node_id[1] == triangle.node_id[2] || triangle.node_id[0] == triangle.node_id[2])
    {
      continue;
    }
    if(numTouched == 3)
    {
      const Node a = finalCoord(triangle.node_id[0]);
      const Node b = finalCoord(triangle.node_id[1]);
      const Node c = finalCoord(triangle.node_id[2]);
      const double abx = static_cast<double>(b.coord[0]) - a.coord[0];
      const double aby = static_cast<double>(b.coord[1]) - a.coord[1];
      const double abz = static_cast<double>(b.coord[2]) - a.coord[2];
      const double acx = static_cast<double>(c.coord[0]) - a.coord[0];
      const double acy = static_cast<double>(c.coord[1]) - a.coord[1];
      const double acz = static_cast<double>(c.coord[2]) - a.coord[2];
      const double crossX = aby * acz - abz * acy;
      const double crossY = abz * acx - abx * acz;
      const double crossZ = abx * acy - aby * acx;
      if(crossX == 0.0 && crossY == 0.0 && crossZ == 0.0)
      {
        continue;
      }
    }
    triangles[static_cast<usize>(survivingCount)] = triangle;
    mCubeID[static_cast<usize>(survivingCount)] = mCubeID[static_cast<usize>(i)];
    survivingCount++;
  }
  triangles.resize(static_cast<usize>(survivingCount));
  mCubeID.resize(static_cast<usize>(survivingCount));
  result.NumFacesRemoved = nTriangle - survivingCount;

  // Pass 3: a touched node is normally still referenced by the wall triangles on either side of the
  // edge, but if every triangle that used it was a chamfer (possible once the Bounding Box Skin prune
  // has removed the walls around it) it is now an orphan and must not be emitted.
  for(const Triangle& triangle : triangles)
  {
    for(const SiteId nodeId : triangle.node_id)
    {
      touched.erase(nodeId);
    }
  }
  for(const SiteId orphan : touched)
  {
    nodeType[static_cast<usize>(orphan)] = M3CNodeType::k_Unused;
    result.SnappedCoords.erase(orphan);
  }
  return result;
}

// -----------------------------------------------------------------------------
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

// Convert a 1-based padded working-grid site to the 0-based index into the original (unpadded)
// cell arrays, or SIZE_MAX if the site is in the ghost shell.
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

// Find a representative original cell for a working label among the 8 corner sites of a marching
// cube. Returns SIZE_MAX if no non-ghost corner carries that label (i.e. the label is exterior).
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

// -----------------------------------------------------------------------------
// Shared finalization for both the whole-volume and sliding-window variants: given the assembled
// triangles/nodeType, promote surface nodes, compact node ids, and write the output TriangleGeom +
// FaceLabels + NodeTypes, then transfer selected arrays and (optionally) repair triangle windings.
// Kept as a free function so it can name the file-local mesh types.
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

  // Promote surface nodes to their exterior variant (+10). A triangle that borders the outside of the
  // volume has exactly one negative feature label (nSpin[0]*nSpin[1] < 0), so each of its nodes lies on
  // the volume boundary. This is the only output-relevant effect of the legacy triangle-side/inner-edge
  // connectivity pass: the per-triangle edge ids, edgePlace flags, and unique inner-edge list it also
  // built never appear in the output (Triangle Geometry + Face Labels + Node Types), so that machinery
  // has been removed.
  for(usize j = 0; j < triangles.size(); j++)
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

  // Sharp Bounding Box Edges: snap the outermost wall rows onto the box edges and drop the chamfer
  // triangles (see sharpenBoundingBoxEdges). Runs on the scratch vectors, so the output TriangleGeom is
  // sized from the surviving count exactly as for the skin prune above.
  SharpEdgeResult sharpEdges;
  if(inputValues->SharpBoundingBoxEdges)
  {
    messageHandler("Sharpening bounding box edges...");
    sharpEdges = sharpenBoundingBoxEdges(triangles, mCubeID, nodeType, 7 * numSites, nodeCoords, dims);
    messageHandler(fmt::format("Sharpened bounding box edges: removed {} chamfer triangles", sharpEdges.NumFacesRemoved));
  }

  const int64 nTriangleFinal = static_cast<int64>(triangles.size());

  // The face-edge segments are no longer needed; release before the memory-heavy output + winding stages.
  std::vector<Segment>().swap(fedges);

  if(shouldCancel)
  {
    return {};
  }

  // --- Stage 5: compact nodes and write the output TriangleGeom --------------
  messageHandler("Writing surface mesh...");
  // Node-id compaction without a dense 7*numSites candidate->id map. A candidate's compacted id is simply
  // the number of real nodes (nodeType > 0) that precede it; we answer that from a coarse per-block
  // prefix over nodeType plus a small in-block scan (saves ~3.8 GB at 512^3 vs a uint32 map). This is
  // valid because the prefix is built here, after the skin prune and the sharp-edge pass have cleared
  // the nodes they retire and the surface-node promotion has added +10 to the rest.
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

  // Compact nodes: scatter coordinates + node types to their new (sequential) ids. Walking candidates
  // in ascending order and emitting with a running counter reproduces assign_new_nodeID's numbering.
  int64 vtxRunning = 0;
  for(SiteId i = 0; i < numCandidateNodes; i++)
  {
    if(nodeType[i] > 0)
    {
      const auto snappedIt = sharpEdges.SnappedCoords.find(i);
      const Node nodeCoord = (snappedIt != sharpEdges.SnappedCoords.end()) ? snappedIt->second : nodeCoords[i];
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

    for(int64 i = 0; i < nTriangleFinal; i++)
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
  std::vector<int8>().swap(nodeType);
  std::vector<int32>().swap(point);
  std::vector<uint32>().swap(nodeBlockBase);

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

  // Reject Feature Ids that collide with this algorithm's internal sentinel space: maxGrainId+1 is
  // signed overflow (undefined behavior) when INT32_MAX is present, and negative Feature Ids collide
  // with M3C's nSpin < 0 ghost convention. This is a mitigation for the underlying sentinel-collision
  // design, not a fix -- see simplnx#1705. Run here (execute), not preflight: a full-volume scan is
  // too expensive to repeat on every GUI parameter edit.
  {
    const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
    Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(featureIdsStore, m_InputValues->FeatureIdsArrayPath, /*rejectMaxInt32=*/true, m_ShouldCancel, m_MessageHandler);
    if(sentinelCheck.invalid())
    {
      return sentinelCheck;
    }
  }

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

// -----------------------------------------------------------------------------
Result<> M3CSurfaceMeshing::runEntireVolume()
{
  // The geometry parameter only accepts ImageGeom (M3C's node coordinates assume uniform cell spacing).
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();

  SizeVec3 gridDims = imageGeom.getDimensions();
  usize dims[3] = {gridDims[0], gridDims[1], gridDims[2]};
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 imgOrigin = imageGeom.getOrigin();
  const float res[3] = {spacing[0], spacing[1], spacing[2]};
  const float origin[3] = {imgOrigin[0], imgOrigin[1], imgOrigin[2]};

  // Always wrap the volume in a ghost layer (NX inputs are not pre-wrapped).
  constexpr bool k_AddSurfaceLayer = true;
  usize fileDim[3] = {dims[0] + 2, dims[1] + 2, dims[2] + 2};
  const usize totalPoints = fileDim[0] * fileDim[1] * fileDim[2];
  // Site count and per-plane site count are 64-bit (SiteId): they index the FeatureId grid and must
  // support volumes with more than 2^31 voxels. (Edge/node id storage is unsigned 32-bit, capping the
  // MESH size near 2^32 elements; the peak memory model still scales with volume - see docs.)
  const SiteId numSites = static_cast<SiteId>(totalPoints);
  const SiteId numSitesPerPlane = static_cast<SiteId>(fileDim[0] * fileDim[1]);

  // Read FeatureIds directly from its DataStore (no full-array copy) into the padded working grid.
  // The FeatureId==0 renumbering happens on the working grid, never on the input array.
  m_MessageHandler("Initializing working grid and ghost layer...");
  std::vector<int32> point(totalPoints + 1, 0);
  const int maxGrainId = initialize_micro(k_AddSurfaceLayer, dims, fileDim, featureIdsStore, point.data());

  // On-demand accessors replace the former full-volume voxCoords, node, and neighbor arrays.
  const SiteCoords siteCoords{fileDim[0], fileDim[1], fileDim[0] * fileDim[1], {res[0], res[1], res[2]}, {origin[0], origin[1], origin[2]}};
  const NodeCoords nodeCoords{siteCoords};
  const NeighborAccessor neighbors{numSites, numSitesPerPlane, static_cast<int>(fileDim[0])};

  // 3 marching squares (top/back/left) per site; node types (7 candidate nodes/site) start Unused (0).
  m_MessageHandler("Initializing candidate nodes and squares...");
  std::vector<Face> squares(static_cast<usize>(3) * numSites);
  std::vector<int8> nodeType(static_cast<usize>(7) * numSites, 0);
  initialize_squares(squares.data(), numSites);

  if(m_ShouldCancel)
  {
    return {};
  }

  // --- Stage 2: face edges ---------------------------------------------------
  m_MessageHandler("Counting face edges...");
  const int64 nFEdge = get_number_fEdges(squares.data(), point.data(), neighbors, numSites, m_ShouldCancel);

  m_MessageHandler("Finding nodes and edges on each square...");
  std::vector<Segment> fedges(static_cast<usize>(nFEdge < 0 ? 0 : nFEdge));
  get_nodes_fEdges(squares.data(), point.data(), neighbors, nodeType.data(), fedges.data(), numSites, numSitesPerPlane, static_cast<int>(fileDim[0]), m_ShouldCancel);

  if(m_ShouldCancel)
  {
    return {};
  }

  // --- Stage 3: triangles ----------------------------------------------------
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

// -----------------------------------------------------------------------------
// Sliding-window (z-slice) variant (the default path). See M3CSurfaceMeshing.hpp: the marching-square
// scratch is held for only two z-slices at a time, so per-site scratch is O(sliceArea) rather than
// O(volume). Produces byte-identical output to runEntireVolume().
Result<> M3CSurfaceMeshing::runWindowed(bool parallel)
{
  // --- Setup (identical to runEntireVolume) ----------------------------------
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

  // Mesh-scale accumulators grow with the output; only the per-site squares are windowed.
  // fedges is the only vector that grows incrementally (push_back during the pass-1 sweep; its
  // capacity is pre-reserved from an early extrapolation -- see the sweep lambda). triangles and
  // mCubeID stay empty until the pass-1 count finishes and are then resized once to the exact size.
  std::vector<int8> nodeType(static_cast<usize>(7) * numSites, 0);
  std::vector<Segment> fedges;
  std::vector<Triangle> triangles;
  std::vector<SiteId> mCubeID;

  // --- Square window: two z-slices of the 3-marching-squares-per-site scratch ------------------
  // squareId = 3*(site-1)+ord. The window holds squares for sites [winBaseSite, winBaseSite + 2*numSitesPerPlane);
  // winIndex maps an absolute squareId to its slot. As the cube sweep crosses into the next slice the
  // window slides forward by one slice (memmove the second slice down, compute the new second slice).
  const SiteId sliceSquares = 3 * numSitesPerPlane; // squares per z-slice
  std::vector<Face> window(static_cast<usize>(2) * sliceSquares);
  SiteId winBaseSite = 1;
  auto winIndex = [&winBaseSite](SiteId squareId) -> usize { return static_cast<usize>(squareId - 3 * (winBaseSite - 1)); };

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
                  // Unconditional promotion to k_Default -- see the matching comment in get_nodes_fEdges.
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

  // The case-table functions (get_number_caseX / get_caseX) FLIP shared face edges in place while
  // tracing loops, so the whole-volume ordering (ALL counts, then ALL generates) is significant for a
  // byte-identical result. We reproduce it with two windowed sweeps over the same persistent fedges:
  //   pass 1 (appendEdges=true,  generate=false): build fedges, set node types, and run every count
  //                                               (applying all count-flips in cube order);
  //   pass 2 (appendEdges=false, generate=true):  rebuild the windowed squares (no fedges append) and
  //                                               generate triangles (applying all generate-flips).
  int64 nTriangle = 0;

  const int64 totalSlices = (numSitesPerPlane > 0) ? (numSites / numSitesPerPlane) : 1;
  const int64 progressStep = std::max<int64>(1, totalSlices / 20); // ~20 progress updates per sweep

  // One-shot fedges capacity estimate: a few slices into the edge-appending pass, extrapolate the
  // final edge count from the per-slice rate seen so far (with a small margin) so the potentially
  // multi-GB fedges vector avoids the repeated geometric reallocations (and their transient memory
  // spikes) that incremental push_back growth would otherwise cost. Never triggers for small volumes.
  bool fedgesReserved = false;
  const int64 reserveAfterSlices = std::max<int64>(4, totalSlices / 16);
  auto maybeReserveFedges = [&](int64 eid) {
    const int64 sliceIdx = winBaseSite / numSitesPerPlane;
    if(fedgesReserved || sliceIdx < reserveAfterSlices || sliceIdx >= totalSlices)
    {
      return;
    }
    fedgesReserved = true;
    // The window is two slices ahead of the slide counter: after N slides the squares (and their
    // appended edges) of slices [1, N + 2] have been computed.
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

      // Slide the window forward so it always covers sites [i, i + numSitesPerPlane].
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
        // Clamp to k_QuadPoint ("4 or more grains") -- see the note in get_number_triangles.
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
    // Experimental multithreaded sweep. The edge stage (computeSquares) runs SERIALLY per slice, so the
    // squares, fedges, and edge-node nodeType are built identically to the serial path (hence identical
    // vertices, FaceLabels, and NodeTypes). The per-cube count/generate work is then parallel across the
    // cubes of each slice: each cube reads the shared squares/fedges but flips a PRIVATE copy of its face
    // edges, so cubes never mutate shared state and no coloring is needed. Because the legacy per-cube
    // loop triangulation depends on cross-cube edge-flip propagation (inherently serial), dropping it
    // yields a DIFFERENT but valid triangulation of the same interfaces -- correct and watertight, but
    // not the same triangles as the serial path. Triangle order still matches (offsets from the count).
    const SiteId lastCube = numSites - numSitesPerPlane;
    const usize numCubes = (lastCube >= 1) ? static_cast<usize>(lastCube) : 0; // cubes are 1..lastCube

    // Per-cube work with a private edge copy (thread-safe). doGenerate=false counts (and sets the
    // body-center node type); doGenerate=true writes triangles at triOffset. Returns the triangle count.
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
        // Clamp to k_QuadPoint ("4 or more grains") -- see the note in get_number_triangles.
        nodeType[BCnode] = static_cast<int8>(std::min(nds, static_cast<int>(M3CNodeType::k_QuadPoint)));
      }
      if(eff <= 0 || nFE <= 2)
      {
        return 0;
      }
      // Private copy of this cube's face edges; local indices are 0..nFE-1 (max ~24, buffer is 64).
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

    // Slide the serial edge-stage window forward until it covers sites [targetBaseSite, +2*NSP).
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
