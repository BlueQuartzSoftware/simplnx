#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryThinningImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKBinaryThinningImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the per-slice sequential
// thinning engine's bit-exact output-VALUE parity gate.
const Uuid k_LegacyBTUuid = *Uuid::FromString("8fcd24cb-769d-400f-97cf-9b4dad1b8cd2");

using BTFilter = BinaryThinningImageFilter;

// The 2D shapes exercised by the parity grid: a solid block (thins to a medial skeleton), a thick plus (a
// junction), a hollow ring (a closed loop that must NOT be broken), and a thick L (a corner junction with two
// endpoints). Together they cover skeleton medial axes, junctions, endpoints, and loops.
enum class Shape
{
  SolidRect,
  Plus,
  Loop,
  LJunction,
  BorderBar, // a 3-px-thick vertical bar flush against x==0 (pins the ZeroFluxNeumann edge handling)
  BorderL    // an L flush against x==0 AND y==0 (foreground touching two borders at once)
};

// Paint @p shape (foreground value @p fg) into z-slice @p z of the flat X-fastest field @p v.
template <class T>
void PaintShape(std::vector<T>& v, Shape shape, usize dimX, usize dimY, usize z, T fg)
{
  auto set = [&](usize x, usize y, T value) {
    if(x < dimX && y < dimY)
    {
      v[rt::FlatIndex(x, y, z, dimX, dimY)] = value;
    }
  };
  auto fillRect = [&](usize x0, usize x1, usize y0, usize y1, T value) {
    for(usize y = y0; y <= y1; ++y)
    {
      for(usize x = x0; x <= x1; ++x)
      {
        set(x, y, value);
      }
    }
  };

  switch(shape)
  {
  case Shape::SolidRect:
    fillRect(3, 11, 3, 11, fg);
    break;
  case Shape::Plus:
    fillRect(6, 8, 2, 12, fg); // vertical arm (3px wide)
    fillRect(2, 12, 6, 8, fg); // horizontal arm (3px wide)
    break;
  case Shape::Loop:
    fillRect(3, 11, 3, 11, fg); // outer solid block
    fillRect(5, 9, 5, 9, T{0}); // carve the interior -> a ring (closed loop)
    break;
  case Shape::LJunction:
    fillRect(3, 5, 3, 11, fg);  // vertical bar (3px wide)
    fillRect(3, 11, 9, 11, fg); // horizontal foot (3px tall)
    break;
  case Shape::BorderBar:
    fillRect(0, 2, 3, 11, fg); // 3-px-thick vertical bar flush against x==0
    break;
  case Shape::BorderL:
    fillRect(0, 2, 0, 11, fg); // vertical arm flush against x==0 (and y==0 at the corner)
    fillRect(0, 11, 0, 2, fg); // horizontal arm flush against y==0 (and x==0 at the corner)
    break;
  }
}

// A single-slice 2D field holding one @p shape (foreground 1).
template <class T>
std::vector<T> MakeShapeField(Shape shape, usize dimX, usize dimY)
{
  std::vector<T> v(dimX * dimY, T{0});
  PaintShape<T>(v, shape, dimX, dimY, 0, T{1});
  return v;
}

// A 2-slice 3D field with a DIFFERENT shape on each z-slice, to lock the per-slice (z-decoupled) behavior:
// slice 0 gets @p s0, slice 1 gets @p s1 (both foreground 1).
template <class T>
std::vector<T> MakeStackedField(Shape s0, Shape s1, usize dimX, usize dimY)
{
  std::vector<T> v(dimX * dimY * 2, T{0});
  PaintShape<T>(v, s0, dimX, dimY, 0, T{1});
  PaintShape<T>(v, s1, dimX, dimY, 1, T{1});
  return v;
}

// A deterministic large thin-slab pattern for the OOC-vs-in-core byte-match gate: isolated 8x8 foreground blocks on
// a 16-pixel grid (gaps between them), repeated on every z-slice. Each block thins quickly, so the streamed path is
// exercised at scale without pathological convergence time.
template <class T>
std::vector<T> MakeSlabPattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ, T{0});
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        if((x % 16 < 8) && (y % 16 < 8))
        {
          v[rt::FlatIndex(x, y, z, dimX, dimY)] = T{1};
        }
      }
    }
  }
  return v;
}

// Sets the standard geom/input/output keys on a shared Arguments (thinning is parameterless), runs preflight +
// execute, and requires both succeed. Reuses BTFilter::k_*_Key for BOTH the new and the legacy ITK filter --
// correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name").
void RunThinning(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// Run new + legacy on the same field and require the outputs match EXACTLY (byte-identical output VALUES -- the
// live-ITK gate on the per-slice sequential thinning engine).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  BTFilter newFilter;
  RunThinning(newFilter, newDs, newInput);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyBTUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunThinning(*legacyFilter, legacyDs, legacyInput);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<T>(newOut, legacyOut); // EXACT -- thinned values must byte-match live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL integer type grid x {2D shapes: solid rect, plus, loop, L-junction} + a 3D
//     stacked case. BinaryThinning's output is SameAsInput (values 0/1); the new filter must reproduce legacy ITK's
//     output VALUES EXACTLY (no tolerance -- see the Parity model). This is the primary gate on the per-slice
//     sequential thinning engine.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::BinaryThinningImageFilter: Live-ITK parity", "[ImageProcessing][BinaryThinningImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyBTUuid) != nullptr);

  constexpr usize DX = 15, DY = 15;

  SECTION("2D solid rectangle")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::SolidRect, DX, DY), DX, DY, 1);
  }
  SECTION("2D plus/cross (junction)")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::Plus, DX, DY), DX, DY, 1);
  }
  SECTION("2D closed loop")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::Loop, DX, DY), DX, DY, 1);
  }
  SECTION("2D L-junction (corner + endpoints)")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::LJunction, DX, DY), DX, DY, 1);
  }
  SECTION("3D stacked (rect on z=0, plus on z=1)")
  {
    RequireParity<T>(MakeStackedField<T>(Shape::SolidRect, Shape::Plus, DX, DY), DX, DY, 2);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
//     rejected (IntegerOnly); a valid integer scalar input produces a SameAsInput output array.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: preflight guards", "[ImageProcessing][BinaryThinningImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    BTFilter filter;
    Arguments args;
    args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float input rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    const DataPath geomPath = inputPath.getParent().getParent();
    BTFilter filter;
    Arguments args;
    args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
  }
  SECTION("scalar integer input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeShapeField<uint8>(Shape::SolidRect, 15, 15);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 15, 15, 1, field);
    BTFilter filter;
    RunThinning(filter, ds, inputPath);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (4) FromSIMPLJson: the geometry/array/name DataPaths (thinning has no algorithm params). The geometry and the
//     input array BOTH read the same SIMPL key "SelectedCellArrayPath"; the output name reads "NewCellArrayName".
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: FromSIMPLJson", "[ImageProcessing][BinaryThinningImageFilter]")
{
  const nlohmann::json json = {
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "ThinnedOut"},
  };
  const Result<Arguments> result = BinaryThinningImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<DataPath>(BTFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(BTFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(BTFilter::k_OutputImageArrayName_Key) == "ThinnedOut");
}

// -----------------------------------------------------------------------------
// (5) Edge-touching Live-ITK parity: foreground FLUSH against the image border pins the per-slice thinning engine's
//     ZeroFluxNeumann (edge-clamp) boundary handling against live ITK. Every shape in test 1 sits >=2px inside the
//     border, so the edge behavior was previously only covered by the OOC-vs-in-core byte-match (test 3), never a
//     live-ITK compare. A 3-px bar flush at x==0 and an L flush against two borders, EXACT vs the legacy ITK filter.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::BinaryThinningImageFilter: edge-touching Live-ITK parity", "[ImageProcessing][BinaryThinningImageFilter]", uint8, int16, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyBTUuid) != nullptr);

  constexpr usize DX = 15, DY = 15;

  SECTION("2D bar flush against x==0")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::BorderBar, DX, DY), DX, DY, 1);
  }
  SECTION("2D L flush against x==0 and y==0")
  {
    RequireParity<T>(MakeShapeField<T>(Shape::BorderL, DX, DY), DX, DY, 1);
  }
  SECTION("3D stacked (bar on z=0, L on z=1)")
  {
    RequireParity<T>(MakeStackedField<T>(Shape::BorderBar, Shape::BorderL, DX, DY), DX, DY, 2);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKBinaryThinningImageTest.cpp on OUR ITK-free filter (BlackDots.png ->
// uint8, SameAsInput). Parameterless filter. (A) DURABLE golden = md5-validity-first (plan Sec.4), (B) LIVE-ITK parity
// = BIT-EXACT (uint8, CompareImages@0.0). Helper pins ForceInCore. The ITK test's second case is "SIMPL Backwards
// Compatibility" (covered separately by our FromSIMPLJson test) and is NOT ported here.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: ITK real-image golden (BinaryThinning)", "[ImageProcessing][ItkGolden][BinaryThinningImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<BinaryThinningImageFilter>("BlackDots.png", "153ad0b2f3658dee3b14ad93d0cfe550");
}
