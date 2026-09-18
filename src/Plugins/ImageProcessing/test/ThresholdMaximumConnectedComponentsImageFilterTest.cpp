#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"

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
// Legacy ITKThresholdMaximumConnectedComponentsImageFilter, created at runtime by UUID (so this target does not
// link ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// bisection auto-threshold's bit-exact binary-output parity gate.
const Uuid k_LegacyTMCCUuid = *Uuid::FromString("dc0f6771-87bf-457f-8430-2d943e039a24");

using TMCCFilter = ThresholdMaximumConnectedComponentsImageFilter;

// Sets the standard geom/input/output keys + the 4 op params on a shared Arguments, runs preflight + execute, and
// requires both succeed. Reuses TMCCFilter::k_*_Key for BOTH the new and the legacy ITK filter -- correct only
// because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "minimum_object_size_in_pixels", "upper_boundary", "inside_value",
// "outside_value").
void RunTMCC(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue,
             const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(minimumObjectSizeInPixels));
  args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(upperBoundary));
  args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(insideValue));
  args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(outsideValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// Run new + legacy on the same field/params and require the uint8 binary outputs match EXACTLY (byte-identical --
// the live-ITK gate on the bisection auto-threshold). Inside/Outside default to ITK's (1, 0); pass a non-default
// combo to gate the count-predicate's InsideValue/OutsideValue handling (ITK counts components on the threshold
// OUTPUT, whose foreground is value != 0, so a non-default Inside/Outside changes which region is maximized).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue = 1u, uint8 outsideValue = 0u)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  TMCCFilter newFilter;
  RunTMCC(newFilter, newDs, newInput, minimumObjectSizeInPixels, upperBoundary, insideValue, outsideValue);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyTMCCUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunTMCC(*legacyFilter, legacyDs, legacyInput, minimumObjectSizeInPixels, upperBoundary, insideValue, outsideValue);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<uint8>(newOut, legacyOut); // EXACT -- binary output must byte-match live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL scalar type grid (incl. float32/float64) x a couple
//     (MinimumObjectSizeInPixels, UpperBoundary) settings x {3D, 2D}, on a MULTI-LEVEL field (several gray levels
//     so the bisection threshold search is non-trivial). ThresholdMaximumConnectedComponents's output is a FIXED
//     uint8 binary image; the new filter must reproduce legacy ITK's output VALUES EXACTLY (no tolerance -- see
//     the Parity model). This is the primary gate on the bisection auto-threshold.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: Live-ITK parity", "[ImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]", uint8, int8, uint16, int16,
                   uint32, int32, uint64, int64, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyTMCCUuid) != nullptr);

  struct Config
  {
    uint32 minimumObjectSizeInPixels;
    float64 upperBoundary;
  };
  const Config cfg = GENERATE(Config{0u, 65536.0}, Config{3u, 120.0});
  CAPTURE(cfg.minimumObjectSizeInPixels, cfg.upperBoundary);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(rt::MakeGradientNoisePattern<T>(DX, DY, DZ), DX, DY, DZ, cfg.minimumObjectSizeInPixels, cfg.upperBoundary);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(rt::MakeGradientNoisePattern<T>(DX, DY, DZ), DX, DY, DZ, cfg.minimumObjectSizeInPixels, cfg.upperBoundary);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: preflight guards", "[ImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]")
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
    TMCCFilter filter;
    Arguments args;
    args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(0u));
    args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
    args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input (incl. float) creates a uint8 output")
  {
    DataStructure ds;
    const std::vector<float32> field = rt::MakeGradientNoisePattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    TMCCFilter filter;
    RunTMCC(filter, ds, inputPath, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/65536.0, /*insideValue=*/1u, /*outsideValue=*/0u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (4) Live-ITK EXACT parity with NON-DEFAULT Inside/Outside values. ITK's ComputeConnectedComponents counts
//     components on the binary-threshold OUTPUT (foreground = value != 0), so InsideValue/OutsideValue drive WHICH
//     region the bisection maximizes -- NOT just the final write. With (Inside=0, Outside=1) ITK maximizes the
//     OUT-of-band region, converging to a DIFFERENT threshold than the default (Inside=1, Outside=0) case; the count
//     predicate must honor this to match live ITK. (Inside=1, Outside=1) is also included: every pixel is foreground
//     so the output is uniformly 1. Each combo must byte-match the legacy ITK filter EXACTLY. This case FAILS if the
//     count predicate ignores Inside/Outside (in-band only) and PASSES once it counts threshold-output nonzero.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: Live-ITK parity, non-default Inside/Outside", "[ImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]",
                   uint8, int16, uint32, float32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyTMCCUuid) != nullptr);

  struct Config
  {
    uint8 insideValue;
    uint8 outsideValue;
  };
  // (0,1): maximize the OUT-of-band region -> different converged threshold than default (the discriminating case).
  // (1,1): every threshold output is nonzero -> exactly one component regardless of threshold, output uniformly 1.
  const Config cfg = GENERATE(Config{0u, 1u}, Config{1u, 1u});
  CAPTURE(cfg.insideValue, cfg.outsideValue);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(rt::MakeGradientNoisePattern<T>(DX, DY, DZ), DX, DY, DZ, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/120.0, cfg.insideValue, cfg.outsideValue);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(rt::MakeGradientNoisePattern<T>(DX, DY, DZ), DX, DY, DZ, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/120.0, cfg.insideValue, cfg.outsideValue);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKThresholdMaximumConnectedComponentsImageTest.cpp on OUR ITK-free
// filter. Output is a FIXED uint8 binary image (AlwaysUInt8); (A) DURABLE golden = md5-validity-first (plan Sec.4),
// (B) LIVE-ITK parity = BIT-EXACT (uint8, CompareImages@0.0). Helper pins ForceInCore.
//   default:    cthead1.png, defaults (MinimumObjectSizeInPixels 0, UpperBoundary 65536, Inside 1, Outside 0).
//   parameters: cthead1.png, MinimumObjectSizeInPixels = 40 AND UpperBoundary = 150.
//   float:      RA-Float.nrrd (float32 input, uint8 output), defaults.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "c84b75c78c33844251a1095d9cbcffb9");
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (parameters)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "27c6cf8494fcc4e414f1c420e7a9ca6f", [](Arguments& args) {
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(40u));
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(150.0));
  });
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("RA-Float.nrrd", "e475b27bd0dd66ede330c4eab93c17e9");
}
