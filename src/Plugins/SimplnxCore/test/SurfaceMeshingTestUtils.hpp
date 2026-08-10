#pragma once

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <algorithm>
#include <map>
#include <utility>

namespace SurfaceMeshingTest
{
using namespace nx::core;

inline constexpr usize k_BoxDim = 12;
inline constexpr usize k_CylinderRadius = 3;
inline constexpr usize k_CylinderCenter = 6;
inline constexpr usize k_CylinderTopZ = 8;

/**
 * @brief Builds a 12x12x12 ImageGeom holding a Z-axis cylinder (Feature Id 1) in a
 * background of Feature Id 0. The cylinder is always inset from the four X/Y walls and
 * from the top; flushWithBottom controls whether it touches the z == 0 wall.
 * @param flushWithBottom When true the cylinder spans z = 0..8, otherwise z = 2..8.
 * @return A DataStructure containing ImageGeom/CellData/FeatureIds.
 */
inline DataStructure CreateCylinderInBox(bool flushWithBottom)
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {k_BoxDim, k_BoxDim, k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());
  featureIdsPtr->fill(0);

  const usize minZ = flushWithBottom ? 0 : 2;
  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();

  for(usize z = minZ; z <= k_CylinderTopZ; z++)
  {
    for(usize y = 0; y < k_BoxDim; y++)
    {
      for(usize x = 0; x < k_BoxDim; x++)
      {
        const float64 dx = static_cast<float64>(x) + 0.5 - static_cast<float64>(k_CylinderCenter);
        const float64 dy = static_cast<float64>(y) + 0.5 - static_cast<float64>(k_CylinderCenter);
        if((dx * dx) + (dy * dy) <= static_cast<float64>(k_CylinderRadius * k_CylinderRadius))
        {
          featureIdsRef[(z * k_BoxDim * k_BoxDim) + (y * k_BoxDim) + x] = 1;
        }
      }
    }
  }

  return dataStructure;
}

/**
 * @brief Builds a 12x12x12 ImageGeom with NO background: eight octant Features numbered
 * 1..8 fill the volume. Omitting the bounding box skin must be a no-op on this data.
 */
inline DataStructure CreateFullyIndexedPolycrystal()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {k_BoxDim, k_BoxDim, k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());

  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  const usize half = k_BoxDim / 2;
  for(usize z = 0; z < k_BoxDim; z++)
  {
    for(usize y = 0; y < k_BoxDim; y++)
    {
      for(usize x = 0; x < k_BoxDim; x++)
      {
        const int32 octant = static_cast<int32>((x >= half ? 1 : 0) + (y >= half ? 2 : 0) + (z >= half ? 4 : 0));
        featureIdsRef[(z * k_BoxDim * k_BoxDim) + (y * k_BoxDim) + x] = octant + 1;
      }
    }
  }

  return dataStructure;
}

/**
 * @brief Builds a 12x12x12 ImageGeom in which every voxel is background (Feature Id 0).
 * Omitting the bounding box skin must yield an empty mesh plus a warning.
 */
inline DataStructure CreateAllBackground()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {k_BoxDim, k_BoxDim, k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());
  featureIdsPtr->fill(0);

  return dataStructure;
}

struct EdgeUseCounts
{
  usize TotalEdges = 0;
  usize EdgesUsedOnce = 0;
  usize EdgesUsedTwice = 0;
  usize EdgesUsedMoreThanTwice = 0;
};

/**
 * @brief Counts how many triangles use each undirected edge of the mesh.
 */
inline EdgeUseCounts CountEdgeUses(const TriangleGeom& triangleGeom)
{
  using VertexPair = std::pair<usize, usize>;
  std::map<VertexPair, usize> edgeUses;

  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const usize numFaces = triangleGeom.getNumberOfFaces();

  for(usize faceIdx = 0; faceIdx < numFaces; faceIdx++)
  {
    const std::array<usize, 3> vertIds = {static_cast<usize>(facesRef[faceIdx * 3 + 0]), static_cast<usize>(facesRef[faceIdx * 3 + 1]), static_cast<usize>(facesRef[faceIdx * 3 + 2])};
    for(usize edgeIdx = 0; edgeIdx < 3; edgeIdx++)
    {
      const usize vertA = vertIds[edgeIdx];
      const usize vertB = vertIds[(edgeIdx + 1) % 3];
      edgeUses[VertexPair{std::min(vertA, vertB), std::max(vertA, vertB)}]++;
    }
  }

  EdgeUseCounts counts;
  counts.TotalEdges = edgeUses.size();
  for(const auto& [edge, useCount] : edgeUses)
  {
    if(useCount == 1)
    {
      counts.EdgesUsedOnce++;
    }
    else if(useCount == 2)
    {
      counts.EdgesUsedTwice++;
    }
    else
    {
      counts.EdgesUsedMoreThanTwice++;
    }
  }
  return counts;
}

/**
 * @brief A mesh is watertight when every edge is shared by exactly two triangles.
 */
inline bool IsWatertight(const TriangleGeom& triangleGeom)
{
  const EdgeUseCounts counts = CountEdgeUses(triangleGeom);
  return counts.TotalEdges > 0 && counts.EdgesUsedOnce == 0 && counts.EdgesUsedMoreThanTwice == 0;
}
} // namespace SurfaceMeshingTest
