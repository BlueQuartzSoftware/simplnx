#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKIsoContourDistanceImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyIsoUuid = *Uuid::FromString("e82fa143-7ac2-4c09-a88c-9ea71d47d594");

using IsoFilter = IsoContourDistanceImageFilter;

void RunIso(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 levelSetValue, float64 farValue, UnitTest::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(IsoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(IsoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(IsoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(IsoFilter::k_LevelSetValue_Key, std::make_any<float64>(levelSetValue));
  args.insertOrAssign(IsoFilter::k_FarValue_Key, std::make_any<float64>(farValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// --- type-safe field builders (values kept in a small non-negative range so every scalar type, incl. uint8/int8, can
//     represent them; signed-only patterns are used only on signed/float types) ---
template <class T>
std::vector<T> MakeRadial(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  const float64 cx = dx / 2.0, cy = dy / 2.0, cz = dz / 2.0;
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
      {
        const float64 d = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz));
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>(std::lround(d));
      }
  return v;
}

template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>(x);
  return v;
}

template <class T>
std::vector<T> MakeSaddle(usize dx, usize dy, usize dz) // signed/float only (produces negatives); |value| <= 121 for D<=12
{
  std::vector<T> v(dx * dy * dz);
  const int64 hx = static_cast<int64>(dx / 2), hy = static_cast<int64>(dy / 2);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
      {
        const int64 xr = static_cast<int64>(x) - hx, yr = static_cast<int64>(y) - hy;
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>(xr * xr - yr * yr);
      }
  return v;
}

template <class T>
std::vector<T> MakeSmoothNoise(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((x + 2 * y + 3 * z + ((x * y + z) % 5)) % 13);
  return v;
}

// Run new + legacy on the same field/params and require byte-exact float32 equality.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 levelSet, float64 farValue)
{
  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyIsoUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunIso(*legacyFilter, legacyDs, legacyInput, levelSet, farValue);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
  {
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure newDs;
    const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
    scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
    IsoFilter newFilter;
    RunIso(newFilter, newDs, newInput, levelSet, farValue, &scope);

    const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
    scope.requireExpectedStore(newOut);
    REQUIRE(newOut.getDataType() == legacyOut.getDataType());
    if constexpr(std::is_same_v<T, float32>)
    {
      UnitTest::CompareDataArrays<float32>(newOut, legacyOut);
    }
    else
    {
      rt::RequireExactFloat32(newOut, legacyOut);
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL type grid (all 10 scalar types incl. float) x {LevelSetValue} x {radial, ramp} x
//     {3D, 2D}. The narrow-band signed distance is a fixed float32 output; the new filter must reproduce legacy ITK
//     EXACTLY (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: Live-ITK exact parity (full type grid)", "[ImageProcessing][IsoContourDistanceImageFilter]", uint8, int8, uint16, int16, uint32,
                   int32, uint64, int64, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyIsoUuid) != nullptr);

  const float64 levelSet = GENERATE(2.5, 6.0);
  CAPTURE(levelSet);

  SECTION("radial 3D")
  {
    RequireParity<T>(MakeRadial<T>(12, 12, 12), 12, 12, 12, levelSet, 10.0);
  }
  SECTION("radial 2D")
  {
    RequireParity<T>(MakeRadial<T>(20, 16, 1), 20, 16, 1, levelSet, 10.0);
  }
  SECTION("ramp 3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, levelSet, 10.0);
  }
  SECTION("ramp 2D")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, levelSet, 10.0);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK parity across MORE field patterns (saddle, border-touching contour, no-crossing, on-contour,
//     smooth-noise) x FarValue variants, on signed/float representative types (patterns can be negative).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: Live-ITK exact parity (field patterns)", "[ImageProcessing][IsoContourDistanceImageFilter]", int16, int32, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyIsoUuid) != nullptr);

  const float64 farValue = GENERATE(10.0, 4.0);
  CAPTURE(farValue);

  SECTION("saddle 3D (levelSet 0)")
  {
    RequireParity<T>(MakeSaddle<T>(12, 12, 5), 12, 12, 5, 0.0, farValue);
  }
  SECTION("saddle 2D (levelSet 0)")
  {
    RequireParity<T>(MakeSaddle<T>(16, 16, 1), 16, 16, 1, 0.0, farValue);
  }
  SECTION("border-touching contour (ramp, levelSet 0.5 -> crossing at the x=0 border)")
  {
    RequireParity<T>(MakeRamp<T>(10, 8, 6), 10, 8, 6, 0.5, farValue);
  }
  SECTION("no crossing (ramp entirely below levelSet)")
  {
    RequireParity<T>(MakeRamp<T>(10, 8, 6), 10, 8, 6, 100.0, farValue);
  }
  SECTION("on-contour band (ramp, integer levelSet lands exactly on a plane)")
  {
    RequireParity<T>(MakeRamp<T>(10, 8, 6), 10, 8, 6, 4.0, farValue);
  }
  SECTION("smooth noise 3D")
  {
    RequireParity<T>(MakeSmoothNoise<T>(12, 11, 10), 12, 11, 10, 6.0, farValue);
  }
  SECTION("smooth noise 2D")
  {
    RequireParity<T>(MakeSmoothNoise<T>(20, 16, 1), 20, 16, 1, 6.0, farValue);
  }
}

// -----------------------------------------------------------------------------
// (3) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry (the spacing math path).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][IsoContourDistanceImageFilter]", int16, float32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyIsoUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRadial<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, DX, DY, DZ, field);
  scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  IsoFilter newFilter;
  RunIso(newFilter, newDs, newInput, 5.0, 10.0, &scope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyIsoUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, DX, DY, DZ, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunIso(*legacyFilter, legacyDs, legacyInput, 5.0, 10.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  if constexpr(std::is_same_v<T, float32>)
  {
    UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath)); // float32: ~1 ULP vs ITK (see above)
  }
  else
  {
    rt::RequireExactFloat32(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath)); // int/float64: bit-exact vs ITK
  }
}

// -----------------------------------------------------------------------------
// (4) Hand-computed 1D crossing: a 5x1x1 ramp {0,1,2,3,4}, levelSet 1.5 -> the crossing is between x=1 and x=2, whose
//     unit-slope signed distances are -0.5 and +0.5; everything else +-farValue. Also parity-checked vs legacy ITK.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: hand-computed 1D crossing", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 1, DZ = 1;
  const std::vector<int32> field = {0, 1, 2, 3, 4};
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, field);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  IsoFilter filter;
  RunIso(filter, ds, inputPath, 1.5, 10.0, &scope);
  const auto& out = ds.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(out.getValue(0) == -10.0f);                      // x=0: far below
  REQUIRE(out.getValue(1) == Approx(-0.5f).epsilon(1e-5)); // x=1
  REQUIRE(out.getValue(2) == Approx(0.5f).epsilon(1e-5));  // x=2
  REQUIRE(out.getValue(3) == 10.0f);                       // x=3: far above
  REQUIRE(out.getValue(4) == 10.0f);                       // x=4: far above
}

// -----------------------------------------------------------------------------
// (5) Preflight: multi-component input rejected; float32 AND float64 scalar input ACCEPTED (this filter is NOT
//     integer-only), with the output created float32.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: preflight accepts float, rejects non-scalar", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& ds, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    IsoFilter filter;
    Arguments args;
    args.insertOrAssign(IsoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(IsoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(IsoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(IsoFilter::k_LevelSetValue_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(IsoFilter::k_FarValue_Key, std::make_any<float64>(10.0));
    return filter.preflight(ds, args);
  };

  SECTION("float32 accepted")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("float64 accepted")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float64>(ds, 6, 6, 1, std::vector<float64>(36, 1.0));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0)); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());

    auto result = runPreflight(ds, vecPath);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("tuple mismatch rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const auto& geom = ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent());
    auto* wrongAM = AttributeMatrix::Create(ds, "WrongCellData", {35}, geom.getId());
    const DataPath wrongPath({"Image Geometry", "WrongCellData", "Short"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, wrongPath, {35}, {1});
    DataArray<int32>::Create(ds, "Short", store, wrongAM->getId());
    const auto result = runPreflight(ds, wrongPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8001);
  }
}

// -----------------------------------------------------------------------------
// (7) FromSIMPLJson: every mapped parameter (both doubles + both DataPaths) at non-default values.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: FromSIMPLJson", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  const nlohmann::json json = {
      {"LevelSetValue", 3.5},
      {"FarValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "IsoOut"},
  };
  const Result<Arguments> result = IsoContourDistanceImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(IsoFilter::k_LevelSetValue_Key) == 3.5);
  REQUIRE(args.value<float64>(IsoFilter::k_FarValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(IsoFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(IsoFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(IsoFilter::k_OutputImageArrayName_Key) == "IsoOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKIsoContourDistanceImageTest.cpp(default): 2th_cthead1.png
// (uint8), LevelSetValue 50.0 (FarValue default), fixed float32 output. The input is INTEGER, whose IsoContour path
// is bit-exact vs live ITK per this filter's own parity tests (the documented ~1 ULP tolerance is for float32 input
// only), so (B) uses RequireExactFloat32; (A) durable ITK baseline compare @ 0.0001.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][IsoContourDistanceImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<IsoContourDistanceImageFilter>(
      "2th_cthead1.png", "BasicFilters_IsoContourDistanceImageFilter_default.nrrd", 0.0001, /*bitExactB=*/true,
      [](Arguments& args) { args.insertOrAssign(IsoContourDistanceImageFilter::k_LevelSetValue_Key, std::make_any<float64>(50.0)); }, &scope);
}
