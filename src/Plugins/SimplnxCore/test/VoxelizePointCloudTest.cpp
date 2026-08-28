#include <catch2/catch.hpp>

#include <limits>

#include "SimplnxCore/Filters/VoxelizePointCloudFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
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

ImageGeom& CreateImageGeom(DataStructure& ds, SizeVec3 dims, const FloatVec3& origin, const FloatVec3& spacing)
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
  args.insertOrAssign(VoxelizePointCloudFilter::k_UseExistingGeometry_Key, std::make_any<bool>(useExisting));
  args.insertOrAssign(VoxelizePointCloudFilter::k_InputPointCloudGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insertOrAssign(VoxelizePointCloudFilter::k_SelectedGridGeometryPath_Key, std::make_any<DataPath>(outputGeomPath));
  args.insertOrAssign(VoxelizePointCloudFilter::k_MaskArrayName_Key, std::make_any<std::string>(maskName));
  args.insertOrAssign(VoxelizePointCloudFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_NewGeomPath));
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
    REQUIRE(mask.getTupleShape() == ShapeType{11, 11, 11}); // {z, y, x} row-major
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

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45980);
  }

  SECTION("TC-A3: Single point — zero-extent bounding box collapses to 1x1x1 geometry")
  {
    // All side lengths are 0, padding is 0. Each dim clamps to 1.
    // origin=(5,5,5), spacing=(1,1,1). Point maps to xRaw=yRaw=zRaw=0 → cell(0,0,0) → flat 0.
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{5.0f, 5.0f, 5.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewGeomPath);
    REQUIRE(newGeom.getDimensions() == SizeVec3{1, 1, 1});

    const DataPath maskPath = k_NewGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(maskPath);
    REQUIRE(mask.getNumberOfTuples() == 1u);
    REQUIRE(mask.getTupleShape() == ShapeType{1, 1, 1});
    REQUIRE(mask[0] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-A4: Planar point cloud (zero Z extent) — collapses Z to 1, XY sized normally")
  {
    // Points all at z=0: side lengths=(2,3,0), padding=(0.002,0.003,0).
    // dims = {ceil(2.004)=3, ceil(3.006)=4, max(1,ceil(0))=1}
    // origin ≈ (-0.002, -0.003, 0.0), sliceSize=3*4=12.
    //   (0,0,0) → xRaw=0.002→xPos=0, yRaw=0.003→yPos=0, zPos=0 → flat 0
    //   (2,0,0) → xRaw=2.002→xPos=2, yPos=0, zPos=0            → flat 2
    //   (0,3,0) → xPos=0, yRaw=3.003→yPos=3, zPos=0            → flat 9
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewGeomPath);
    REQUIRE(newGeom.getDimensions() == SizeVec3{3, 4, 1});

    const DataPath maskPath = k_NewGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(maskPath);
    REQUIRE(mask.getNumberOfTuples() == 12u);
    REQUIRE(mask.getTupleShape() == ShapeType{1, 4, 3}); // {z=1, y=4, x=3} — distinguishes {12} from {1,4,3}
    REQUIRE(CountMarked(dataStructure, maskPath) == 3u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[2] == 1u);
    REQUIRE(mask[9] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-A5: Cloud far from origin — sub-ULP padding is rescued by nextafter expansion")
  {
    // At 1e7f the float32 ULP is 1.0, so 0.1% of a 2.0f extent = 0.002f rounds
    // to zero when added to the bounding-box faces.  Without the nextafter fix the
    // padded extent stays at 2.0f, dims={2,2,2}, and the max-boundary point hits
    // xRaw==dims[0] and is silently excluded.  With nextafter:
    //   minPoint ≈ 9999999.0f  (one ULP below origMin)
    //   maxPoint ≈ 10000003.0f (one ULP above origMax)
    //   distance = 4.0f → dims = {4,4,4}
    //
    // sliceSize = 4*4 = 16.  flat = z*16 + y*4 + x.
    //   (1e7,   1e7,   1e7  ) → cell(1,1,1) → flat 21  (1*16 + 1*4 + 1)
    //   (1e7+2, 1e7+2, 1e7+2) → cell(3,3,3) → flat 63  (3*16 + 3*4 + 3)
    constexpr float32 k_Base = 10000000.0f; // 1e7 — ULP = 1.0 at this magnitude
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{k_Base, k_Base, k_Base}, {k_Base + 2.0f, k_Base + 2.0f, k_Base + 2.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(k_NewGeomPath);
    REQUIRE(newGeom.getDimensions() == SizeVec3{4, 4, 4});

    const DataPath maskPath = k_NewGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(maskPath);

    REQUIRE(CountMarked(dataStructure, maskPath) == 2u);
    REQUIRE(mask[21] == 1u);
    REQUIRE(mask[63] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-A6: Extent that would require petabyte allocation yields clean error -45982")
  {
    // spacing defaults to {1,1,1}.  Two points 30000 units apart → dims ≈ {30030,30030,30030}.
    // Product = ~2.7e13 cells: fits in usize (no integer-overflow UB) but is 27 TB — no system
    // can satisfy this allocation.  bad_alloc is caught in ResizeImageGeom and returned as -45982
    // rather than propagating as an unhandled exception or producing heap corruption.
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.0f, 0.0f, 0.0f}, {30000.0f, 30000.0f, 30000.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45982);
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

  SECTION("TC-B9: Negative non-zero origin and anisotropic spacing — flat indices are correct")
  {
    // dims={6,3,8}, origin={-5,-3,-1}, spacing={0.5,2.0,0.25}
    // sliceSize = 6*3 = 18.  flat = z*18 + y*6 + x.
    //
    // cell formula: pos[i] = floor((pt[i] - origin[i]) / spacing[i])
    //
    //   (-5.0,-3.0,-1.0): xRaw=0/0.5=0, yRaw=0/2.0=0, zRaw=0/0.25=0 → flat   0
    //   (-3.5,-1.0,-0.25): xRaw=1.5/0.5=3, yRaw=2.0/2.0=1, zRaw=0.75/0.25=3 → flat  63  (3*18 + 1*6 + 3)
    //   (-2.5, 1.0, 0.5): xRaw=2.5/0.5=5, yRaw=4.0/2.0=2, zRaw=1.5/0.25=6  → flat 125  (6*18 + 2*6 + 5)
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {6, 3, 8}, {-5.0f, -3.0f, -1.0f}, {0.5f, 2.0f, 0.25f});
    CreatePointCloud(dataStructure, {{-5.0f, -3.0f, -1.0f}, {-3.5f, -1.0f, -0.25f}, {-2.5f, 1.0f, 0.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath maskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(maskPath);

    REQUIRE(CountMarked(dataStructure, maskPath) == 3u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[63] == 1u);
    REQUIRE(mask[125] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B10: Non-cubic grid — transposed flat-index formula is detectable")
  {
    // All previous TC-B grids are cubic (NxNxN), where many index transpositions cancel.
    // dims={7,3,5} (W≠H≠D): sliceSize=7*3=21, flat = z*21 + y*7 + x.
    // A bug using dims[1]=3 as the y-stride gives flat 48 for point 2 (not 52);
    // a bug using dims[1]*dims[2]=15 as the z-stride gives flat 40 (not 52).
    //
    //   (0.5, 0.5, 0.5) → cell(0,0,0) → flat   0
    //   (3.5, 1.5, 2.5) → cell(3,1,2) → flat  52  (2*21 + 1*7 + 3)
    //   (6.5, 2.5, 4.5) → cell(6,2,4) → flat 104  (4*21 + 2*7 + 6)
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {7, 3, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}, {3.5f, 1.5f, 2.5f}, {6.5f, 2.5f, 4.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 3u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[52] == 1u);
    REQUIRE(mask[104] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-B11: NaN and +Inf vertex coordinates are skipped without OOB write")
  {
    // IEEE 754: (NaN >= 0.0f) == false → negated guard fires → skip.
    //           (inf <  dimsXf) == false → negated guard fires → skip.
    // Neither bad vertex must set a mask bit or produce an out-of-bounds store write.
    // Only (2.5,2.5,2.5) → cell(2,2,2) → flat 2*25+2*5+2 = 62 is expected.
    const float32 k_NaN = std::numeric_limits<float32>::quiet_NaN();
    const float32 k_Inf = std::numeric_limits<float32>::infinity();

    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {5, 5, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    CreatePointCloud(dataStructure, {{k_NaN, k_NaN, k_NaN}, {k_Inf, k_Inf, k_Inf}, {2.5f, 2.5f, 2.5f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 1u);
    REQUIRE(mask[62] == 1u);

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

  SECTION("TC-C5: Custom mask name creates the right array and leaves the default absent")
  {
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, xB, yB, zB);
    CreatePointCloud(dataStructure, {{0.5f, 1.0f, 2.0f}});

    const std::string customName = "CustomMask";
    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath, customName));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath customMaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(customName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(customMaskPath));
    REQUIRE(dataStructure.getDataRefAs<UInt8Array>(customMaskPath)[0] == 1u);

    REQUIRE(dataStructure.getData(k_MaskPath) == nullptr);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("TC-C6: Non-cubic RectGridGeom — mask tuple shape matches {nz, ny, nx}")
  {
    // nx=4, ny=2, nz=3 → 24 cells.  Shape {3,2,4} distinguishes {nz,ny,nx} from a flat
    // {24} or a transposed {4,2,3}.  All existing C grids are 3×3×3 so this is the first
    // test where a wrong axis ordering in the shape is detectable.
    //
    // xBounds={0,1,2,3,4}: 4 unit cells
    // yBounds={0,2,4}:     2 cells of width 2
    // zBounds={0,1,3,6}:   3 non-uniform cells
    // sliceSize = nx*ny = 4*2 = 8.  flat = (zPos-1)*8 + (yPos-1)*4 + (xPos-1).
    //
    //   (0.5,1.0,0.5): xPos=1→cell 0, yPos=1→cell 0, zPos=1→cell 0 → flat  0
    //   (2.5,3.0,2.0): xPos=3→cell 2, yPos=2→cell 1, zPos=2→cell 1 → flat 14  (1*8+1*4+2)
    DataStructure dataStructure;
    CreateRectGridGeom(dataStructure, {0.0f, 1.0f, 2.0f, 3.0f, 4.0f}, {0.0f, 2.0f, 4.0f}, {0.0f, 1.0f, 3.0f, 6.0f});
    CreatePointCloud(dataStructure, {{0.5f, 1.0f, 0.5f}, {2.5f, 3.0f, 2.0f}});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask.getNumberOfTuples() == 24u);
    REQUIRE(mask.getTupleShape() == ShapeType{3, 2, 4}); // {nz, ny, nx}
    REQUIRE(CountMarked(dataStructure, k_MaskPath) == 2u);
    REQUIRE(mask[0] == 1u);
    REQUIRE(mask[14] == 1u);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Path D — Higher-dimensional source geometry
// ═════════════════════════════════════════════════════════════════════════════
TEST_CASE("SimplnxCore::VoxelizePointCloudFilter: TriangleGeom source", "[SimplnxCore][VoxelizePointCloudFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("TC-D1: TriangleGeom vertex positions voxelize identically to the same positions from a VertexGeom")
  {
    // Verifies that getVerticesRef() reaches the correct store through the full
    // INodeGeometry0D inheritance chain for a 2-D geometry.  Triangle connectivity
    // is intentionally omitted — the filter uses only vertex positions.
    //
    // 5×5×5 grid (origin=0, spacing=1).  flat = z*25 + y*5 + x.
    //   (0.5,0.5,0.5) → cell(0,0,0) → flat   0
    //   (2.5,1.5,3.5) → cell(2,1,3) → flat  82  (3*25 + 1*5 + 2)
    //   (4.5,4.5,4.5) → cell(4,4,4) → flat 124  (4*25 + 4*5 + 4)
    DataStructure dataStructure;
    CreateImageGeom(dataStructure, {5, 5, 5}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    auto* triGeom = TriangleGeom::Create(dataStructure, k_PointCloudName);
    auto* verts = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "SharedVertexList", {3}, {3}, triGeom->getId());
    (*verts)[0] = 0.5f; (*verts)[1] = 0.5f; (*verts)[2] = 0.5f;
    (*verts)[3] = 2.5f; (*verts)[4] = 1.5f; (*verts)[5] = 3.5f;
    (*verts)[6] = 4.5f; (*verts)[7] = 4.5f; (*verts)[8] = 4.5f;
    triGeom->setVertices(*verts);

    const DataPath k_MaskPath = k_GridGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName).createChildPath(k_DefaultMaskName);

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
}

// ═════════════════════════════════════════════════════════════════════════════
// Path P — Preflight validation error paths
// ═════════════════════════════════════════════════════════════════════════════
TEST_CASE("SimplnxCore::VoxelizePointCloudFilter: Preflight validation", "[SimplnxCore][VoxelizePointCloudFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("TC-P1: Point cloud geometry with no shared vertex list triggers error -45985")
  {
    DataStructure dataStructure;
    VertexGeom::Create(dataStructure, k_PointCloudName); // no setVertices call

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45985);
  }

  SECTION("TC-P2: Vertex list with 2 components per vertex triggers error -45989")
  {
    DataStructure dataStructure;
    auto* geom = VertexGeom::Create(dataStructure, k_PointCloudName);
    auto* verts = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "SharedVertexList", {2}, {2}, geom->getId());
    geom->setVertices(*verts);

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(false));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45989);
  }

  SECTION("TC-P3: Existing ImageGeom with zero X spacing triggers error -45983")
  {
    // Spacing check fires before cell AM check, so no cell AM is needed to isolate -45983.
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}});

    auto* geom = ImageGeom::Create(dataStructure, k_GridGeomName);
    geom->setDimensions({3, 3, 3});
    geom->setSpacing({0.0f, 1.0f, 1.0f});
    geom->setOrigin({0.0f, 0.0f, 0.0f});

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45983);
  }

  SECTION("TC-P4: Existing ImageGeom with no cell AttributeMatrix triggers error -45984")
  {
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}});

    auto* geom = ImageGeom::Create(dataStructure, k_GridGeomName);
    geom->setDimensions({3, 3, 3});
    geom->setSpacing({1.0f, 1.0f, 1.0f});
    geom->setOrigin({0.0f, 0.0f, 0.0f});
    // No cell AM assigned

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45984);
  }

  SECTION("TC-P5: Existing RectGridGeom with no bounds arrays triggers error -45986")
  {
    // Cell AM is required so preflight reaches the bounds null check (-45984 must not fire first).
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}});

    auto* geom = RectGridGeom::Create(dataStructure, k_GridGeomName);
    geom->setDimensions({2, 2, 2});
    auto* cellAM = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, {2, 2, 2}, geom->getId());
    geom->setCellData(*cellAM);
    // No bounds arrays set

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45986);
  }

  SECTION("TC-P6: Existing RectGridGeom with X bounds length mismatched to dims triggers error -45987")
  {
    // dims={2,2,2} requires bounds size==3 per axis; X gets size 2 to trigger the mismatch.
    DataStructure dataStructure;
    CreatePointCloud(dataStructure, {{0.5f, 0.5f, 0.5f}});

    auto* geom = RectGridGeom::Create(dataStructure, k_GridGeomName);
    geom->setDimensions({2, 2, 2});

    auto makeArr = [&](const std::string& name, usize n) {
      return Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, name, {n}, {1}, geom->getId());
    };
    geom->setXBoundsId(makeArr("xBounds", 2)->getId()); // wrong: 2 instead of 3
    geom->setYBoundsId(makeArr("yBounds", 3)->getId());
    geom->setZBoundsId(makeArr("zBounds", 3)->getId());

    auto* cellAM = AttributeMatrix::Create(dataStructure, ImageGeom::k_CellAttributeMatrixName, {2, 2, 2}, geom->getId());
    geom->setCellData(*cellAM);

    VoxelizePointCloudFilter filter;
    auto executeResult = filter.execute(dataStructure, MakeArgs(true, k_GridGeomPath));

    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -45987);
  }
}
