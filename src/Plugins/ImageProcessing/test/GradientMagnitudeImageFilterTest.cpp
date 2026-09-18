#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GradientMagnitudeImageFilter.hpp"

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
// Legacy ITKGradientMagnitudeImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the central-difference engine's
// bit-exact parity gate.
const Uuid k_LegacyGradMagUuid = *Uuid::FromString("719df7b2-8db2-43eb-a40c-a015982eec08");

using GradMagFilter = GradientMagnitudeImageFilter;

// Sets the standard geom/input/output keys + the UseImageSpacing flag on a shared Arguments, runs preflight + execute,
// and requires both succeed. Reuses GradMagFilter::k_*_Key for BOTH the new and the legacy ITK filter -- correct only
// because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "use_image_spacing").
void RunGradMag(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool useImageSpacing, UnitTest::AlgorithmTestScope* algorithmTestScope = nullptr,
                const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeFilter = [&]() { return filter.execute(ds, args); };
  auto executeResult = algorithmTestScope == nullptr ? executeFilter() : algorithmTestScope->execute(executeFilter);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8) so
// the full 10-type parity grid can reuse it. Varies along all three axes so the gradient magnitude is non-trivial.
template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((3 * x + 2 * y + z) % 17);
      }
    }
  }
  return v;
}

// Run new + legacy on the same field/params and require the float32 gradient-magnitude outputs match. The per-voxel
// central-difference stencil is deterministic and thread-count-independent, so ITK matches bit-exact for ALL input
// types, float32 included (Parity model). Uses rt::RequireExactFloat32 for every type.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, bool useImageSpacing, UnitTest::AlgorithmTestScope* algorithmTestScope = nullptr)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  GradMagFilter newFilter;
  RunGradMag(newFilter, newDs, newInput, useImageSpacing, algorithmTestScope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyGradMagUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunGradMag(*legacyFilter, legacyDs, legacyInput, useImageSpacing);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  rt::RequireExactFloat32(newOut, legacyOut); // bit-exact vs live ITK for every input type (deterministic stencil)
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL type grid (all 10 scalar types incl. float) x {UseImageSpacing on/off} x {3D, 2D}.
//     The gradient magnitude is a fixed float32 output; the new filter must reproduce legacy ITK EXACTLY (no
//     tolerance). This is the primary gate on the central-difference engine.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: Live-ITK parity", "[ImageProcessing][GradientMagnitudeImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64,
                   float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyGradMagUuid) != nullptr);

  const bool useImageSpacing = GENERATE(false, true);
  CAPTURE(useImageSpacing);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, useImageSpacing);
  }
  SECTION("2D (Z==1)")
  {
    const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, useImageSpacing, &algorithmTestScope);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry, UseImageSpacing ON (the 1/spacing scaling
//     path), on signed/float representative types.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][GradientMagnitudeImageFilter]", int16, int32, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyGradMagUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, DX, DY, DZ, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  GradMagFilter newFilter;
  RunGradMag(newFilter, newDs, newInput, /*useImageSpacing=*/true);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyGradMagUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, DX, DY, DZ, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunGradMag(*legacyFilter, legacyDs, legacyInput, /*useImageSpacing=*/true);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExactFloat32(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath)); // bit-exact vs live ITK
}

// -----------------------------------------------------------------------------
// (3) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a FIXED
//     float32 output array (AlwaysFloat32), regardless of the input element type.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: preflight guards", "[ImageProcessing][GradientMagnitudeImageFilter]")
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
    GradMagFilter filter;
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a float32 output")
  {
    const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, field);
    GradMagFilter filter;
    RunGradMag(filter, ds, inputPath, /*useImageSpacing=*/false, &algorithmTestScope);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
  SECTION("zero spacing rejected when UseImageSpacing is on")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 6);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 6, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    GradMagFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(true));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_GradientMagnitudeZeroSpacing);
  }
  SECTION("zero spacing accepted when UseImageSpacing is off (spacing ignored)")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 6);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 6, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    GradMagFilter filter;
    RunGradMag(filter, ds, inputPath, /*useImageSpacing=*/false); // spacing ignored -> the degenerate spacing is harmless
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson: the UseImageSpacing bool and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: FromSIMPLJson", "[ImageProcessing][GradientMagnitudeImageFilter]")
{
  const nlohmann::json json = {
      {"UseImageSpacing", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "GradMagOut"},
  };
  const Result<Arguments> result = GradientMagnitudeImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<bool>(GradMagFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(GradMagFilter::k_OutputImageArrayName_Key) == "GradMagOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- "default" case. Duplicates ITKGradientMagnitudeImageTest.cpp(default) on OUR
// ITK-free filter: read RA-Float.nrrd (float32) through OUR reader, run OUR filter with the ITK case's params (all
// defaults; UseImageSpacing defaults true in both filters), then apply the plan's two oracles --
//   (A) DURABLE golden: compare to ITK's committed baseline BasicFilters_GradientMagnitudeImageFilter_default.nrrd at
//       the ITK test's tolerance 1e-05 via ip_golden::CompareImages;
//   (B) LIVE-ITK parity: the legacy ITK filter on the SAME input, BIT-EXACT (this filter's established parity class --
//       the synthetic parity tests above use rt::RequireExactFloat32). Output is a fixed float32 (AlwaysFloat32).
// The whole case is pinned ForceInCore by the helper (the legacy ITK filter bad_casts an OOC store).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][GradientMagnitudeImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientMagnitudeImageFilter>("RA-Float.nrrd", "BasicFilters_GradientMagnitudeImageFilter_default.nrrd", /*tolerance=*/1e-05, /*bitExactB=*/true);
}
