#pragma once

#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace SurfaceMeshingTest
{
using namespace nx::core;

inline constexpr usize k_BoxDim = 12;
inline constexpr usize k_CylinderRadius = 3;
inline constexpr usize k_CylinderCenter = 6;
inline constexpr usize k_CylinderTopZ = 8;

// The ImageGeom/CellData/FeatureIds layout every builder in this file produces.
inline const DataPath k_ImageGeomPath({"ImageGeom"});
inline const DataPath k_FeatureIdsPath({"ImageGeom", "CellData", "FeatureIds"});

/**
 * @brief Builds a 12x12x12 ImageGeom holding a Z-axis cylinder (Feature Id 1) in a
 * background of Feature Id 0. The cylinder is always inset from the four X/Y walls and
 * from the top; flushWithBottom controls whether it touches the z == 0 wall.
 * @param flushWithBottom When true the cylinder spans z = 0..8, otherwise z = 2..8.
 * @param spacing ImageGeom voxel spacing. Defaults to {1,1,1} (isotropic) so existing callers
 * are unaffected; pass an anisotropic value to exercise axis-specific spacing bugs.
 * @return A DataStructure containing ImageGeom/CellData/FeatureIds.
 */
inline DataStructure CreateCylinderInBox(bool flushWithBottom, FloatVec3 spacing = {1.0F, 1.0F, 1.0F})
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {k_BoxDim, k_BoxDim, k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing(spacing);
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);
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
  imageGeomPtr->setCellData(*cellAMPtr);
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
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());
  featureIdsPtr->fill(0);

  return dataStructure;
}

/**
 * @brief Builds a 12x12x12 ImageGeom where Feature Id 1 fills the entire volume EXCEPT for a small
 * interior pocket of Feature Id 0 (background), fully enclosed away from all six bounding-box walls
 * (interior porosity). Every wall voxel is Feature 1, so no bounding-box wall face is
 * background-backed: omitting the bounding box skin must prune zero faces and warn (code -56342),
 * even though the volume is full of Feature Id 0 internally. This is the counterexample that
 * distinguishes "no wall face is background-backed" from "the volume has no background voxels" --
 * CreateFullyIndexedPolycrystal() covers the latter, this covers the former without the latter.
 */
inline DataStructure CreateEnclosedPorosity()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {k_BoxDim, k_BoxDim, k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());
  featureIdsPtr->fill(1);

  // A 4x4x4 pocket of background centered in the box, at least two voxels inset from every wall on
  // every axis so it can never be mistaken for a wall-adjacent (and therefore prunable) voxel.
  constexpr usize k_PocketMin = 4;
  constexpr usize k_PocketMax = 7;
  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  for(usize z = k_PocketMin; z <= k_PocketMax; z++)
  {
    for(usize y = k_PocketMin; y <= k_PocketMax; y++)
    {
      for(usize x = k_PocketMin; x <= k_PocketMax; x++)
      {
        featureIdsRef[(z * k_BoxDim * k_BoxDim) + (y * k_BoxDim) + x] = 0;
      }
    }
  }

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

/**
 * @brief Result of running a surface meshing filter: the DataStructure it produced, plus the
 * paths to the created Triangle Geometry and its Face Labels array.
 */
struct MeshResult
{
  DataStructure Structure;
  DataPath TriangleGeomPath;
  DataPath FaceLabelsPath;
};

/**
 * @brief Names the Cell Feature Ids parameter key for a surface mesher filter type. QuickSurfaceMesh
 * and SurfaceNets both name it k_CellFeatureIdsArrayPath_Key, so that is the default; M3CSurfaceMeshingFilter
 * names the same concept k_FeatureIdsArrayPath_Key, so it gets an explicit specialization below. This lets
 * RunMesher stay a single shared implementation instead of forking per-mesher.
 */
template <class FilterT>
struct FeatureIdsKeyTrait
{
  static constexpr auto k_Key = FilterT::k_CellFeatureIdsArrayPath_Key;
};

template <>
struct FeatureIdsKeyTrait<M3CSurfaceMeshingFilter>
{
  static constexpr auto k_Key = M3CSurfaceMeshingFilter::k_FeatureIdsArrayPath_Key;
};

/**
 * @brief Builds the Arguments common to every surface mesher (QuickSurfaceMeshFilter,
 * SurfaceNetsFilter, M3CSurfaceMeshingFilter), then invokes addExtraArgs to set the
 * mesher-specific ones (e.g. Fix Problem Voxels, smoothing options). Shared by RunMesher and
 * RunMesherRaw so the argument list is defined exactly once.
 * @param triangleGeomPath Path at which to create the Triangle Geometry.
 * @param boundingBoxSkinMode Value for the Bounding Box Skin parameter (a BoundingBoxSkinMode
 * value; ChoicesParameter::ValueType so bool literals from pre-existing call sites still convert).
 * @param addExtraArgs Callback that inserts the mesher-specific arguments into the Arguments object.
 * @param repairWinding Value for the Repair Triangle Winding parameter. Defaults to false (matching
 * every pre-existing call site); pass true to exercise the shipped default configuration instead.
 */
template <class FilterT, class ExtraArgsFn>
inline Arguments BuildMesherArgs(const DataPath& triangleGeomPath, ChoicesParameter::ValueType boundingBoxSkinMode, ExtraArgsFn addExtraArgs, bool repairWinding = false)
{
  Arguments args;
  args.insertOrAssign(FilterT::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(FeatureIdsKeyTrait<FilterT>::k_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FilterT::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(FilterT::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(FilterT::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(FilterT::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
  args.insertOrAssign(FilterT::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
  args.insertOrAssign(FilterT::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
  args.insertOrAssign(FilterT::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
  args.insertOrAssign(FilterT::k_RepairTriangleWinding_Key, std::make_any<bool>(repairWinding));
  args.insertOrAssign(FilterT::k_BoundingBoxSkinMode_Key, std::make_any<ChoicesParameter::ValueType>(boundingBoxSkinMode));

  addExtraArgs(args);
  return args;
}

/**
 * @brief Runs a surface meshing filter (QuickSurfaceMeshFilter, SurfaceNetsFilter, or
 * M3CSurfaceMeshingFilter) on the given DataStructure, setting every argument that is common to
 * all three meshers, then invoking addExtraArgs to set the mesher-specific ones (e.g. Fix Problem
 * Voxels, smoothing options). Every mesher shares the same parameter key names for the arguments
 * set here (feature ids excepted -- see FeatureIdsKeyTrait), so callers only need to supply the
 * pieces that differ. Asserts both preflight and execute succeed; for cases that legitimately
 * warn or fail (e.g. an all-background input), use RunMesherRaw instead.
 * @param dataStructure Input DataStructure, e.g. from CreateCylinderInBox().
 * @param triangleGeomPath Path at which to create the Triangle Geometry.
 * @param boundingBoxSkinMode Value for the Bounding Box Skin parameter (a BoundingBoxSkinMode
 * value; ChoicesParameter::ValueType so bool literals from pre-existing call sites still convert).
 * @param addExtraArgs Callback that inserts the mesher-specific arguments into the Arguments object.
 */
template <class FilterT, class ExtraArgsFn>
inline MeshResult RunMesher(DataStructure&& dataStructure, const DataPath& triangleGeomPath, ChoicesParameter::ValueType boundingBoxSkinMode, ExtraArgsFn addExtraArgs)
{
  MeshResult meshResult;
  meshResult.Structure = std::move(dataStructure);
  meshResult.TriangleGeomPath = triangleGeomPath;
  meshResult.FaceLabelsPath = triangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");

  FilterT filter;
  Arguments args = BuildMesherArgs<FilterT>(triangleGeomPath, boundingBoxSkinMode, addExtraArgs);

  auto preflightResult = filter.preflight(meshResult.Structure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(meshResult.Structure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  return meshResult;
}

/**
 * @brief Like RunMesher, but operates on a caller-owned DataStructure (by reference, so the
 * caller keeps it around for inspection) and returns the execute Result<> unchecked instead of
 * asserting validity. This lets degenerate-input tests (e.g. an all-background volume) assert on
 * warnings/errors instead of aborting via SIMPLNX_RESULT_REQUIRE_VALID. Preflight is still
 * asserted valid, since a preflight failure here would mean the test itself is set up wrong, not
 * the runtime condition under test.
 * @param dataStructure Input DataStructure, e.g. from CreateAllBackground(). Modified in place.
 * @param triangleGeomPath Path at which to create the Triangle Geometry.
 * @param boundingBoxSkinMode Value for the Bounding Box Skin parameter (a BoundingBoxSkinMode
 * value; ChoicesParameter::ValueType so bool literals from pre-existing call sites still convert).
 * @param addExtraArgs Callback that inserts the mesher-specific arguments into the Arguments object.
 * @param repairWinding Value for the Repair Triangle Winding parameter. Defaults to false (matching
 * every pre-existing call site); pass true to exercise the shipped default configuration instead.
 */
template <class FilterT, class ExtraArgsFn>
inline Result<> RunMesherRaw(DataStructure& dataStructure, const DataPath& triangleGeomPath, ChoicesParameter::ValueType boundingBoxSkinMode, ExtraArgsFn addExtraArgs, bool repairWinding = false)
{
  FilterT filter;
  Arguments args = BuildMesherArgs<FilterT>(triangleGeomPath, boundingBoxSkinMode, addExtraArgs, repairWinding);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  return filter.execute(dataStructure, args).result;
}

/**
 * @brief Collects the distinct {comp0, comp1} Face Labels pairs present in a mesh result.
 */
inline std::set<std::pair<int32, int32>> CollectLabelPairs(const MeshResult& meshResult)
{
  REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<Int32Array>(meshResult.FaceLabelsPath));
  const auto& faceLabelsRef = meshResult.Structure.getDataRefAs<Int32Array>(meshResult.FaceLabelsPath).getDataStoreRef();
  std::set<std::pair<int32, int32>> labelPairs;
  for(usize i = 0; i < faceLabelsRef.getNumberOfTuples(); i++)
  {
    labelPairs.insert({faceLabelsRef[i * 2], faceLabelsRef[i * 2 + 1]});
  }
  return labelPairs;
}
} // namespace SurfaceMeshingTest
