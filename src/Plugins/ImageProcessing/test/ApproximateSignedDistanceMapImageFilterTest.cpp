#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"

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
// Legacy ITKApproximateSignedDistanceMapImageFilter, created at runtime by UUID.
const Uuid k_LegacyAsdUuid = *Uuid::FromString("87ed0d3a-c394-4bb5-ac7f-6cc746984b09");

using AsdFilter = ApproximateSignedDistanceMapImageFilter;

void RunAsd(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 insideValue, float64 outsideValue, UnitTest::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(AsdFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(AsdFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AsdFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(AsdFilter::k_InsideValue_Key, std::make_any<float64>(insideValue));
  args.insertOrAssign(AsdFilter::k_OutsideValue_Key, std::make_any<float64>(outsideValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A filled blob (interior = insideVal) on a background (outsideVal), via the shared hole-pattern builder.
template <class T>
std::vector<T> MakeBlob(usize dx, usize dy, usize dz, T insideVal, T outsideVal)
{
  return rt::MakeHolePattern<T>(dx, dy, dz, outsideVal, insideVal);
}

// Several integer regions (values 0..3 in quadrants) -> exercises non-binary integer input (ITK thresholds at the
// (inside+outside)/2 iso-level, so >=1 is "inside").
template <class T>
std::vector<T> MakeMultiLabel(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
      {
        const T label = static_cast<T>(((x >= dx / 2) ? 1 : 0) + ((y >= dy / 2) ? 2 : 0));
        v[rt::FlatIndex(x, y, z, dx, dy)] = label;
      }
  return v;
}

// A block that touches the image border (exercises the boundary handling).
template <class T>
std::vector<T> MakeBorderMask(usize dx, usize dy, usize dz, T insideVal, T outsideVal)
{
  std::vector<T> v(dx * dy * dz, outsideVal);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        if(x < dx / 2) // half-space touching the x=0 border
          v[rt::FlatIndex(x, y, z, dx, dy)] = insideVal;
  return v;
}

template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 inside, float64 outside)
{
  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyAsdUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunAsd(*legacyFilter, legacyDs, legacyInput, inside, outside);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
  {
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure newDs;
    const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
    scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
    AsdFilter newFilter;
    RunAsd(newFilter, newDs, newInput, inside, outside, &scope);
    const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
    scope.requireExpectedStore(newOut);
    REQUIRE(newOut.getDataType() == legacyOut.getDataType());
    UnitTest::CompareDataArrays<float32>(newOut, legacyOut);
  }
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, full integer-type grid x {Inside,Outside} x {blob 3D, blob 2D}. Fixed float32 output;
//     the new filter must reproduce legacy ITK EXACTLY.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: Live-ITK exact parity grid", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]", uint8, int8, uint16, int16,
                   uint32, int32, uint64, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyAsdUuid) != nullptr);

  SECTION("binary (1,0) blob 3D")
  {
    RequireParity<T>(MakeBlob<T>(12, 12, 12, T{1}, T{0}), 12, 12, 12, 1.0, 0.0);
  }
  SECTION("binary (1,0) blob 2D")
  {
    RequireParity<T>(MakeBlob<T>(20, 16, 1, T{1}, T{0}), 20, 16, 1, 1.0, 0.0);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK parity across Inside/Outside variants + mask patterns (multi-label, border, uniform) on
//     representative types.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: Live-ITK parity variants", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]", uint8, int16, int32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyAsdUuid) != nullptr);

  SECTION("inside=255 outside=0")
  {
    RequireParity<T>(MakeBlob<T>(12, 12, 10, T{255}, T{0}), 12, 12, 10, 255.0, 0.0);
  }
  SECTION("swapped inside=0 outside=1")
  {
    RequireParity<T>(MakeBlob<T>(12, 12, 10, T{0}, T{1}), 12, 12, 10, 0.0, 1.0);
  }
  SECTION("inside=2 outside=5")
  {
    RequireParity<T>(MakeBlob<T>(12, 12, 10, T{2}, T{5}), 12, 12, 10, 2.0, 5.0);
  }
  SECTION("multi-label 3D")
  {
    RequireParity<T>(MakeMultiLabel<T>(12, 12, 8), 12, 12, 8, 1.0, 0.0);
  }
  SECTION("multi-label 2D")
  {
    RequireParity<T>(MakeMultiLabel<T>(20, 16, 1), 20, 16, 1, 1.0, 0.0);
  }
  SECTION("border-touching")
  {
    RequireParity<T>(MakeBorderMask<T>(12, 10, 8, T{1}, T{0}), 12, 10, 8, 1.0, 0.0);
  }
  SECTION("uniform (no boundary)")
  {
    RequireParity<T>(std::vector<T>(12 * 12 * 4, T{1}), 12, 12, 4, 1.0, 0.0);
  }
}

// -----------------------------------------------------------------------------
// (3) Live-ITK parity with a non-unit (anisotropic) spacing geometry (the spacing/voxel quirk of the composite).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyAsdUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<uint8> field = MakeBlob<uint8>(DX, DY, DZ, uint8{1}, uint8{0});
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, field);
  scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  AsdFilter newFilter;
  RunAsd(newFilter, newDs, newInput, 1.0, 0.0, &scope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyAsdUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunAsd(*legacyFilter, legacyDs, legacyInput, 1.0, 0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (4) Sign orientation on a filled square: interior negative, exterior positive (and equal to legacy ITK).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: sign orientation", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize D = 9;
  std::vector<int32> field(D * D, 0);
  for(usize y = 3; y <= 5; ++y)
  {
    for(usize x = 3; x <= 5; ++x)
    {
      field[rt::FlatIndex(x, y, 0, D, D)] = 1; // filled 3x3 object
    }
  }
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, D, D, 1, field);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  AsdFilter filter;
  RunAsd(filter, ds, inputPath, 1.0, 0.0, &scope);
  const auto& out = ds.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(4, 4, 0, D, D)) < 0.0f); // deep interior
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, D, D)) > 0.0f); // far exterior corner
}

// -----------------------------------------------------------------------------
// (5) Preflight: a float32 input is rejected (IntegerOnly); a multi-component input is rejected; integer input creates
//     a float32 output.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: preflight guards", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& ds, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    AsdFilter filter;
    Arguments args;
    args.insertOrAssign(AsdFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(AsdFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(AsdFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(AsdFilter::k_InsideValue_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(AsdFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
    return filter.preflight(ds, args);
  };

  SECTION("float32 rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(ds, inputPath).outputActions.invalid());
  }
  SECTION("integer accepted, output float32")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, std::vector<uint8>(36, uint8{1}));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
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
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: FromSIMPLJson", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InsideValue", 3.0},
      {"OutsideValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "AsdOut"},
  };
  const Result<Arguments> result = ApproximateSignedDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(AsdFilter::k_InsideValue_Key) == 3.0);
  REQUIRE(args.value<float64>(AsdFilter::k_OutsideValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(AsdFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(AsdFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(AsdFilter::k_OutputImageArrayName_Key) == "AsdOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKApproximateSignedDistanceMapImageTest.cpp:
//   default:        2th_cthead1.png (uint8), ITK-default params.
//   modified_parms: 2th_cthead1.png, InsideValue 100, OutsideValue 0.
// Fixed float32 output; (B) live-ITK at the filter's established tolerant (CompareDataArrays<float32>) parity class +
// (A) durable ITK baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_default.nrrd", 0.01, false, rt::NoExtraParams(),
                                                                               &scope);
}

TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (modified_parms)", "[ImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>(
      "2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_modified_parms.nrrd", 0.01, /*bitExactB=*/false,
      [](Arguments& args) {
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_InsideValue_Key, std::make_any<float64>(100.0));
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
      },
      &scope);
}
