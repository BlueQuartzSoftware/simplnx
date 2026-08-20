#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/VoxelizePointCloudFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
const std::string k_PointCloudName = "PointCloud";
const std::string k_GridGeomName = "GridGeom";
const std::string k_NewGeomName = "NewImageGeom";
const std::string k_DefaultMaskName = "Shared Voxels Mask";

const DataPath k_VertexGeomPath({k_PointCloudName});
const DataPath k_GridGeomPath({k_GridGeomName});
const DataPath k_NewGeomPath({k_NewGeomName});

VertexGeom& CreatePointCloud(DataStructure& ds, const std::vector<std::array<float32, 3>>& points)
{
  auto* geom = VertexGeom::Create(ds, k_PointCloudName);
  auto* verts = Float32Array::CreateWithStore<DataStore<float32>>(ds, "SharedVertexList", {points.size()}, {3}, geom->getId());
  geom->setVertices(*verts);
  for(usize i = 0; i < points.size(); i++)
  {
    (*verts)[i * 3 + 0] = points[i][0];
    (*verts)[i * 3 + 1] = points[i][1];
    (*verts)[i * 3 + 2] = points[i][2];
  }
  return *geom;
}

// Empty point cloud results in an invalid bounding box
VertexGeom& CreateEmptyPointCloud(DataStructure& ds)
{
  auto* geom = VertexGeom::Create(ds, k_PointCloudName);
  auto* verts = Float32Array::CreateWithStore<DataStore<float32>>(ds, "SharedVertexList", {0}, {3}, geom->getId());
  geom->setVertices(*verts);
  return *geom;
}

ImageGeom& CreateImageGeom(DataStructure& ds, SizeVec3 dims, FloatVec3 origin, FloatVec3 spacing)
{
  auto* geom = ImageGeom::Create(ds, k_GridGeomName);
  geom->setDimensions(dims);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* cellAM = AttributeMatrix::Create(ds, ImageGeom::k_CellAttributeMatrixName, {dims[2], dims[1], dims[0]}, geom->getId());
  geom->setCellData(*cellAM);
  return *geom;
}

RectGridGeom& CreateRectGridGeom(DataStructure& ds, const std::vector<float32>& xB, const std::vector<float32>& yB, const std::vector<float32>& zB)
{
  const usize nx = xB.size() - 1;
  const usize ny = yB.size() - 1;
  const usize nz = zB.size() - 1;

  auto* geom = RectGridGeom::Create(ds, k_GridGeomName);
  geom->setDimensions(SizeVec3{nx, ny, nz});

  auto makeArr = [&](const std::string& name, const std::vector<float32>& vals) -> Float32Array* {
    auto* arr = Float32Array::CreateWithStore<DataStore<float32>>(ds, name, {vals.size()}, {1}, geom->getId());
    for(usize i = 0; i < vals.size(); i++)
    {
      (*arr)[i] = vals[i];
    }
    return arr;
  };

  geom->setXBoundsId(makeArr("xBounds", xB)->getId());
  geom->setYBoundsId(makeArr("yBounds", yB)->getId());
  geom->setZBoundsId(makeArr("zBounds", zB)->getId());

  auto* cellAM = AttributeMatrix::Create(ds, ImageGeom::k_CellAttributeMatrixName, {nz, ny, nx}, geom->getId());
  geom->setCellData(*cellAM);
  return *geom;
}

// Counts cells where mask is active
usize CountMarked(DataStructure& ds, const DataPath& maskPath)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(maskPath));
  const auto& mask = ds.getDataRefAs<UInt8Array>(maskPath);
  usize count = 0;
  for(usize i = 0; i < mask.getNumberOfTuples(); i++)
  {
    if(mask[i] != 0u)
    {
      count++;
    }
  }
  return count;
}

Arguments MakeArgs(bool useExisting, const DataPath& outputGeomPath = DataPath{}, const std::string& maskName = k_DefaultMaskName)
{
  Arguments args;
  args.insertOrAssign(VoxelizePointCloudFilter::k_UseExistingGeom_Key, std::make_any<bool>(useExisting));
  args.insertOrAssign(VoxelizePointCloudFilter::k_PointCloudGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insertOrAssign(VoxelizePointCloudFilter::k_OutputGeometryPath_Key, std::make_any<DataPath>(outputGeomPath));
  args.insertOrAssign(VoxelizePointCloudFilter::k_MaskName_Key, std::make_any<std::string>(maskName));
  args.insertOrAssign(VoxelizePointCloudFilter::k_NewGeometryPath_Key, std::make_any<DataPath>(k_NewGeomPath));
  return args;
}
} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Path A — UseExistingGeom = false  (new ImageGeom auto-sized from point cloud)
// ═════════════════════════════════════════════════════════════════════════════
TEST_CASE("SimplnxCore::VoxelizePointCloudFilter: New ImageGeom", "[SimplnxCore][VoxelizePointCloudFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("TC-A1: All points — including the max-boundary corner — are included after auto-sizing")
  {
    // bounding box: min=(0,0,0) max=(10,10,10) → sideLength=10, padding=0.01
    // origin ≈ (-0.01,-0.01,-0.01), distance≈10.02, dims=ceil(10.02)={11,11,11}
    //
    // Flat index = z*11*11 + y*11 + x  (dims={11,11,11})
    //   (0,0,0)   → cell (0,0,0)   → flat 0
    //   (2,3,4)   → cell (2,3,4)   → flat 4*121+3*11+2  = 519
    //   (7,5,8)   → cell (7,5,8)   → flat 8*121+5*11+7  = 1030
    //   (10,10,10)→ cell (10,10,10)→ flat 10*121+10*11+10 = 1330
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {
                                        {0.0f, 0.0f, 0.0f},
                                        {10.0f, 10.0f, 10.0f},
                                        {2.0f, 3.0f, 4.0f},
                                        {7.0f, 5.0f, 8.0f},
                                    });

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_NewGeomPath));
    const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewGeomPath);
    REQUIRE(newGeom.getDimensions() == SizeVec3{11, 11, 11});

    const DataPath maskPath = k_NewGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(maskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(maskPath);

    REQUIRE(mask.getNumberOfTuples() == 1331u);          // 11*11*11
    REQUIRE(CountMarked(dataStructure, maskPath) == 4u); // all 4 points included

    REQUIRE(mask[0] == 1u);    // (0,0,0)
    REQUIRE(mask[519] == 1u);  // (2,3,4)
    REQUIRE(mask[1030] == 1u); // (7,5,8)
    REQUIRE(mask[1330] == 1u); // (10,10,10)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-A2: Empty point cloud triggers invalid-bounding-box error")
  {
    DataStructure dataStructure;
    CreateEmptyPointCloud(dataStructure);

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));

    REQUIRE(executeResult.result.invalid());
    REQUIRE(!executeResult.result.errors().empty());
    REQUIRE(executeResult.result.errors()[0].code == -45980);
  }

  SECTION("TC-A3: Single point — zero-extent bounding box is an error (0-dim geometry is invalid)")
  {
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{5.0f, 5.0f, 5.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));

    REQUIRE(executeResult.result.invalid());
    REQUIRE(!executeResult.result.errors().empty());
    REQUIRE(executeResult.result.errors()[0].code == -45981);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Path B — UseExistingGeom = true, destination is an ImageGeom
// ═════════════════════════════════════════════════════════════════════════════
TEST_CASE("SimplnxCore::VoxelizePointCloudFilter: Existing ImageGeom", "[SimplnxCore][VoxelizePointCloudFilter]")
{
  UnitTest::LoadPlugins();

  // Flat index in a W×H×D ImageGeom: flat = z*(W*H) + y*W + x  (dims = {W,H,D})
  const DataPath k_MaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);

  SECTION("TC-B1: Three interior points map to the correct flat indices")
  {
    // 5×5×5 grid (origin=0, spacing=1).  Flat = z*25 + y*5 + x.
    //   (0.5,0.5,0.5) → cell(0,0,0) → flat 0
    //   (2.5,1.5,3.5) → cell(2,1,3) → flat 3*25+1*5+2 = 82
    //   (4.5,4.5,4.5) → cell(4,4,4) → flat 4*25+4*5+4 = 124
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {5, 5, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}, {2.5f, 1.5f, 3.5f}, {4.5f, 4.5f, 4.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 3u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[82] == 1u);
    REQUIRE(mask[124] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B2: All points past the positive boundary — mask all-zeros, no crash")
  {
    // 3×3×3 grid [0,3)³ — every point has at least one coordinate ≥ 3.0
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {3, 3, 3}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{3.0f, 1.0f, 1.0f}, {5.0f, 5.0f, 5.0f}, {1.0f, 1.0f, 3.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 0u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B3: All points before the origin — mask all-zeros, no crash")
  {
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {3, 3, 3}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{-1.0f, -1.0f, -1.0f}, {-0.5f, -0.5f, -0.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 0u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B4: Mixed inside and outside — only inside voxels marked")
  {
    // 3×3×3 grid [0,3)³.  Flat = z*9 + y*3 + x.
    //   (0.5,0.5,0.5)  → inside  → flat 0
    //   (2.5,2.5,2.5)  → inside  → flat 2*9+2*3+2 = 26
    //   (3.5,1.0,1.0)  → outside (x≥3)
    //   (-1.0,1.0,1.0) → outside (x<0)
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {3, 3, 3}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}, {2.5f, 2.5f, 2.5f}, {3.5f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 2u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[26] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B5: Multiple points in the same voxel — mask value stays 1, not 3")
  {
    // 4×4×4 grid.  Cell(1,1,1) → flat = 1*16 + 1*4 + 1 = 21.
    // Three points all floor to the same cell; setValue is idempotent.
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {4, 4, 4}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{1.1f, 1.1f, 1.1f}, {1.5f, 1.5f, 1.5f}, {1.9f, 1.9f, 1.9f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 1u);
    REQUIRE(mask[21] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B6: Point exactly at the origin — included in cell 0")
  {
    // floor((0.0 - 0.0) / 1.0) = 0; 0 < dims[0]=5 → included.
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {5, 5, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{0.0f, 0.0f, 0.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 1u);
    REQUIRE(mask[0] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B7: Point exactly at the max boundary — excluded (half-open interval)")
  {
    // 5×5×5 grid (origin=0, spacing=1).  Max boundary = 5.0 in each dim.
    // floor((5.0 - 0.0) / 1.0) = 5; 5 >= dims[0]=5 → skip.
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {5, 5, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{5.0f, 5.0f, 5.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 0u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B8: Custom mask name creates the right array and leaves the default absent")
  {
    const std::string customName = "CustomMask";
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {3, 3, 3}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath, customName));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath customMaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(customName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(customMaskPath));
    REQUIRE(dataStructure.getDataRefAs<UInt8Array>(customMaskPath)[0] == 1u);

    // The default name must NOT have been created
    const DataPath defaultMaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    REQUIRE(dataStructure.getData(defaultMaskPath) == nullptr);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Path C — UseExistingGeom = true, destination is a RectGridGeom
// ═════════════════════════════════════════════════════════════════════════════
TEST_CASE("SimplnxCore::VoxelizePointCloudFilter: Existing RectGridGeom", "[SimplnxCore][VoxelizePointCloudFilter]")
{
  UnitTest::LoadPlugins();

  // 3×3×3 rect grid used by most C tests.
  // x-bounds={0,1,3,6}: cells [0,1) [1,3) [3,6)
  // y-bounds={0,2,5,9}: cells [0,2) [2,5) [5,9)
  // z-bounds={0,4,7,10}: cells [0,4) [4,7) [7,10)
  //
  // upper_bound gives 1-based xPos; cell = xPos-1.
  // Flat index: (zPos-1)*nx*ny + (yPos-1)*nx + (xPos-1)  with nx=ny=nz=3.
  const std::vector<float32> xB = {0.0f, 1.0f, 3.0f, 6.0f};
  const std::vector<float32> yB = {0.0f, 2.0f, 5.0f, 9.0f};
  const std::vector<float32> zB = {0.0f, 4.0f, 7.0f, 10.0f};

  const DataPath k_MaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);

  SECTION("TC-C1: Two interior points map to correct cells in a non-uniform grid")
  {
    // (0.5, 1.0, 2.0): x→xPos=1, y→yPos=1, z→zPos=1 → cell(0,0,0) → flat 0
    // (4.0, 6.0, 8.0): x→xPos=3, y→yPos=3, z→zPos=3 → cell(2,2,2) → flat 26
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, xB, yB, zB);
    CreatePointCloud(dataStructure, {{0.5f, 1.0f, 2.0f}, {4.0f, 6.0f, 8.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 2u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[26] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-C2: Point before the origin — excluded via xPos==0 guard")
  {
    // upper_bound([0,1,3,6], -1.0) returns begin() → xPos=0 → guard fires.
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, xB, yB, zB);
    CreatePointCloud(dataStructure, {{-1.0f, -1.0f, -1.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 0u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-C3: Point past the max bound — excluded via xPos>dims guard")
  {
    // upper_bound([0,1,3,6], 100.0) returns end() → xPos=4 > dims[0]=3 → guard fires.
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, xB, yB, zB);
    CreatePointCloud(dataStructure, {{100.0f, 100.0f, 100.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 0u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-C4: Point exactly on an internal cell boundary — belongs to the upper cell")
  {
    // Uniform 3×3×3 grid: each axis has bounds {0,1,2,3}.
    // Point at x=1.0 exactly: upper_bound({0,1,2,3}, 1.0) finds first element > 1.0 = 2.0
    // → index 2 → xPos=2 → cell 1 (second cell, not cell 0).
    //
    // Point: (1.0, 0.5, 0.5)
    //   x→xPos=2 (cell 1), y→yPos=1 (cell 0), z→zPos=1 (cell 0)
    //   flat = (1-1)*9 + (1-1)*3 + (2-1) = 1
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, {0.0f, 1.0f, 2.0f, 3.0f}, {0.0f, 1.0f, 2.0f, 3.0f}, {0.0f, 1.0f, 2.0f, 3.0f});
    CreatePointCloud(dataStructure, {{1.0f, 0.5f, 0.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 1u);
    REQUIRE(mask[1] == 1u); // cell(1,0,0), not cell(0,0,0)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
