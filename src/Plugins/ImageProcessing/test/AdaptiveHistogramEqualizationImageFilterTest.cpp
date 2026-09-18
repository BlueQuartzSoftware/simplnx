#include "AdaptiveHistogramEqualizationFilterTestUtils.hpp"
#include "ItkGoldenTestUtils.hpp"

#include "ImageProcessing/Filters/AdaptiveHistogramEqualizationImageFilter.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace nx::core;
namespace aht = ahe_test;

namespace
{
// Legacy ITKAdaptiveHistogramEqualizationImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing).
const Uuid k_LegacyAdaptiveHistogramEqualizationUuid = *Uuid::FromString("ea3e7439-8327-4190-8ff7-49ecc321718f");

constexpr int32 k_NonScalarInput = -8540;

// Parity configs: default + boundary-stressing (alpha/beta extremes) on 3D uniform, 3D non-cubic, and 2D.
const std::vector<aht::AheCase> k_ParityConfigs = {{"3D r{2,2,2} a=0.3 b=0.3", 0.3f, 0.3f, {2, 2, 2}, 12, 12, 12},
                                                   {"3D r{1,1,1} a=0.0 b=0.0", 0.0f, 0.0f, {1, 1, 1}, 12, 12, 12},
                                                   {"3D r{2,1,1} a=1.0 b=0.0", 1.0f, 0.0f, {2, 1, 1}, 12, 12, 12},
                                                   {"3D r{3,3,3} a=0.6 b=0.2", 0.6f, 0.2f, {3, 3, 3}, 14, 13, 12},
                                                   {"2D Z=1 r{3,3,0} a=0.3 b=0.3", 0.3f, 0.3f, {3, 3, 0}, 20, 16, 1}};
} // namespace

// (1) Tolerant live-ITK parity across representative scalar types. Integer: |a-b|<=1 on <=0.5% of voxels;
//     float: |a-b| <= 1e-2 + 1e-4*|b|. Bit-for-bit parity is impossible (ITK sums over an unordered_map in
//     hash order). Tolerances are the plan seeds and were NOT loosened: against real ITK the integer
//     differing-voxel fraction was measured at <= 0.0017 across all configs (~3x margin under the 0.005 cap,
//     far below a systematic-error level), and every float voxel was within the atol/rtol band.
TEMPLATE_TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: Tolerant legacy parity", "[ImageProcessing][AdaptiveHistogramEqualizationImageFilter]", uint8, int16, float32, float64)
{
  using T = TestType;
  if constexpr(std::is_integral_v<T>)
  {
    aht::RunAheParityGrid<AdaptiveHistogramEqualizationImageFilter, T>(k_LegacyAdaptiveHistogramEqualizationUuid, k_ParityConfigs, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
  }
  else
  {
    aht::RunAheParityGrid<AdaptiveHistogramEqualizationImageFilter, T>(k_LegacyAdaptiveHistogramEqualizationUuid, k_ParityConfigs, /*atol=*/1e-2, /*rtol=*/1e-4, /*maxDiffFraction=*/0.0);
  }
}

// (2) Independent double-precision oracle on a small in-memory image. Real-disk out-of-core coverage for this
//     filter is retired to the generic SimplnxOoc store tests (the adopted model); this case keeps the UNIQUE
//     oracle-based correctness check -- the tolerant legacy-parity grid above only compares against ITK, never
//     against an independent reference. The AHE engine is single-implementation (no DispatchAlgorithm, so it
//     records no algorithm-path execution), which means it CANNOT use UnitTest::AlgorithmTestScope: the scope's
//     executeFilter witness requires a recorded path and would fail with an observed count of 0. The case is
//     therefore a plain ForceInCore in-memory correctness check.
TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: Independent oracle (in-memory)", "[ImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize dimX = 24;
  constexpr usize dimY = 20;
  constexpr usize dimZ = 16;
  const std::vector<uint32> radius = {1, 1, 1};
  constexpr float32 alpha = 0.3f;
  constexpr float32 beta = 0.3f;

  DataStructure ds;
  const DataPath inputPath = aht::BuildAheImage<uint8>(ds, dimX, dimY, dimZ);

  AdaptiveHistogramEqualizationImageFilter filter;
  aht::RunAheFilter<AdaptiveHistogramEqualizationImageFilter>(filter, ds, inputPath, alpha, beta, radius);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  std::vector<uint8> actual(outStore.getSize());
  for(usize i = 0; i < actual.size(); ++i)
  {
    actual[i] = outStore.getValue(i);
  }

  const std::vector<uint8> input = aht::MakeAhePattern<uint8>(dimX, dimY, dimZ);
  const std::vector<uint8> expected = aht::AdaptiveHistogramEqualizationOracle<uint8>(input, dimX, dimY, dimZ, {1, 1, 1}, alpha, beta);
  aht::RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// (3) Preflight: multi-component input is rejected (scalar-only); a valid scalar input preflights cleanly and
//     an unsupported (non-numeric) type is rejected by the shared type-membership guard.
TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: Preflight scalar/type guard", "[ImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  AdaptiveHistogramEqualizationImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{5, 5, 5}));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(0.3f));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(0.3f));
    return args;
  };

  SECTION("scalar input preflights cleanly")
  {
    DataStructure ds;
    const DataPath inputPath = aht::BuildAheImage<uint8>(ds, 8, 8, 8);
    auto result = filter.preflight(ds, makeArgs(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("multi-component input is rejected")
  {
    // Build a 2-component array under the geometry and point the filter at it.
    DataStructure ds;
    const DataPath scalarPath = aht::BuildAheImage<uint8>(ds, 8, 8, 8); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<uint8>(ds, vecPath, {8, 8, 8}, {2});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<uint8>::Create(ds, "Vec", vecStore, cellAM.getId());

    auto result = filter.preflight(ds, makeArgs(vecPath));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_NonScalarInput);
  }
}

// (4) FromSIMPLJson maps the legacy SIMPL keys (Radius/Alpha/Beta + array paths) to the new parameter keys.
TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: FromSIMPLJson", "[ImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  const nlohmann::json json = {{"Radius", {{"x", 4}, {"y", 3}, {"z", 2}}},
                               {"Alpha", 0.7},
                               {"Beta", 0.1},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Equalized"}};

  Result<Arguments> result = AdaptiveHistogramEqualizationImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<VectorParameter<uint32>::ValueType>(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key) == std::vector<uint32>{4, 3, 2});
  REQUIRE(args.value<float32>(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key) == Approx(0.7f));
  REQUIRE(args.value<float32>(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key) == Approx(0.1f));
  REQUIRE(args.value<std::string>(AdaptiveHistogramEqualizationImageFilter::k_OutputImageArrayName_Key) == "Equalized");
}

namespace
{
// Materialize a uint8 scalar array into a flat vector (the AHE golden case is ForceInCore).
std::vector<uint8> MaterializeUint8(DataStructure& ds, const DataPath& path)
{
  const auto& store = ds.getDataRefAs<DataArray<uint8>>(path).getDataStoreRef();
  std::vector<uint8> out(store.getSize());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = store.getValue(i);
  }
  return out;
}

// Reports the tolerant divergence (max |delta|, differing-voxel fraction) between two uint8 arrays via WARN, so
// the measured coexistence divergence is always recorded even when the case passes.
void ReportDivergence(const std::string& tag, const std::vector<uint8>& a, const std::vector<uint8>& b, double allowedFraction)
{
  usize nDiff = 0;
  usize maxDiff = 0;
  for(usize i = 0; i < a.size(); ++i)
  {
    const usize d = static_cast<usize>(std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i])));
    if(d != 0)
    {
      ++nDiff;
    }
    maxDiff = std::max(maxDiff, d);
  }
  WARN(
      fmt::format("{}: maxAbsDiff={} differingFraction={} (allowed |delta|<=1 on <= {})", tag, maxDiff, a.empty() ? 0.0 : static_cast<double>(nDiff) / static_cast<double>(a.size()), allowedFraction));
}

// Runs the SimplnxCore ColorToGrayScale filter (luminosity algorithm, ITK RGB weights) on the input array,
// exactly as the legacy ITKAdaptiveHistogramEqualizationImageTest does: sf4.png is a palette/RGB image and AHE
// runs on the grayscale. Returns the created grayscale array path <cellData>/GrayScale_<inputName>.
DataPath RunColorToGrayScale(DataStructure& ds, const DataPath& inputDataPath)
{
  const Uuid k_ColorToGrayScaleUuid = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
  IFilter::UniquePointer filter = Application::Instance()->getFilterList()->createFilter(k_ColorToGrayScaleUuid);
  REQUIRE(filter != nullptr);

  Arguments args;
  args.insertOrAssign("conversion_algorithm_index", std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign("color_channel", std::make_any<Int32Parameter::ValueType>(0));
  args.insertOrAssign("input_data_array_paths", std::make_any<MultiArraySelectionParameter::ValueType>({inputDataPath}));
  args.insertOrAssign("output_array_prefix", std::make_any<StringParameter::ValueType>("GrayScale_"));

  auto preflightResult = filter->preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter->execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  return inputDataPath.getParent().createChildPath(fmt::format("GrayScale_{}", inputDataPath.getTargetName()));
}

// ITK-sourced real-image golden for AHE (plan Task 6). AHE is TOLERANT: ITK accumulates a neighborhood over an
// unordered_map in hash order, so it is not bit-reproducible even ITK-vs-ITK on a different build, and our
// ITK-free engine is not bit-identical to ITK. The ITK source 2e-3 / 1e-5 CompareImages tolerance is effectively
// BIT-EXACT for a uint8 image (any 1-level difference exceeds it) and is only valid against ITK own baseline. So
// (A) the durable golden compares the output to the committed .png baseline with the SAME tolerant band the
// filter live-ITK parity uses (|delta|<=1 per voxel on a small fraction of voxels), and (B) live-ITK parity is
// likewise tolerant. Input (sf4.png, palette/RGB) and baseline are read through OUR readers; the input is
// converted to grayscale by the same ColorToGrayScale filter the ITK test uses. The whole case is ForceInCore
// (the legacy ITK filter bad_casts an out-of-core store).
void RunAheItkGolden(const std::string& baselineFile, float32 alpha, float32 beta, const std::vector<uint32>& radius, bool assertBaselineGolden, double aMaxDiffFraction, double bMaxDiffFraction)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath rgbInput = cellData.createChildPath("Input");

  // --- read sf4.png (palette/RGB) and convert to grayscale, exactly as the ITK test ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("sf4.png"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  const DataPath grayInput = RunColorToGrayScale(ds, rgbInput);

  const auto makeArgs = [&](const std::string& outputName) {
    Arguments args;
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(grayInput));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(alpha));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(beta));
    return args;
  };

  // --- run OUR AHE on the grayscale input ---
  {
    AdaptiveHistogramEqualizationImageFilter filter;
    const Arguments args = makeArgs("Output");
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const DataPath output = cellData.createChildPath("Output");

  // --- (A) DURABLE golden: tolerant compare vs the committed .png baseline ---
  const DataPath bGeom({"Baseline Geometry"});
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const std::vector<uint8> ourOut = MaterializeUint8(ds, output);
  const std::vector<uint8> baselineData = MaterializeUint8(ds, baseline);
  REQUIRE(ourOut.size() == baselineData.size());
  ReportDivergence(fmt::format("AHE (A) baseline='{}'", baselineFile), ourOut, baselineData, aMaxDiffFraction);
  // The committed defaults baseline (ITKAdaptiveHistogramEqualizationFilterTest.png) is STALE: it does not
  // correspond to the (defaults) parameters, so ITKAdaptiveHistogramEqualizationImageTest.cpp comments out its
  // own baseline assertion (line 124). We mirror that: measured divergence is recorded above, but (A) is only
  // asserted for the (histo) case whose committed baseline is valid (maxAbsDiff 1, ~1 voxel). The (defaults)
  // case relies on (B) live-ITK parity instead.
  if(assertBaselineGolden)
  {
    aht::RequireAheClose<uint8>(ourOut, baselineData, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/aMaxDiffFraction);
  }

  // --- (B) LIVE-ITK tolerant parity (coexistence only) on the SAME grayscale input ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  const Arguments itkArgs = makeArgs("ItkOutput");
  const Result<> itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const std::vector<uint8> itkOut = MaterializeUint8(ds, cellData.createChildPath("ItkOutput"));
  ReportDivergence("AHE (B) live-ITK", ourOut, itkOut, bMaxDiffFraction);
  aht::RequireAheClose<uint8>(ourOut, itkOut, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/bMaxDiffFraction);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 6) -- duplicate ITKAdaptiveHistogramEqualizationImageTest.cpp's
// (defaults) and (histo) cases on OUR ITK-free filter. AHE is TOLERANT (not bit-reproducible: ITK accumulates a
// neighborhood over an unordered_map in hash order). Tolerant band |delta|<=1 per voxel on <= 0.5% of voxels --
// the filter's established live-ITK parity class (matches the sibling parity grid's 0.005).
//   - (defaults): the committed baseline is stale (ITK comments out its own assertion), so this case is a
//     (B)-only live-ITK parity gate; the (A) divergence vs the stale baseline is recorded via WARN.
//   - (histo): committed baseline is valid (measured maxAbsDiff 1) -- asserts BOTH (A) baseline and (B) live-ITK.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AdaptiveHistogramEqualizationImageFilter]")
{
  RunAheItkGolden("BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest.png", /*alpha=*/0.5f, /*beta=*/0.5f, /*radius=*/{10, 19, 10}, /*assertBaselineGolden=*/false, /*aMaxDiffFraction=*/0.005,
                  /*bMaxDiffFraction=*/0.005);
}
TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationImageFilter: ITK real-image golden (histo)", "[ImageProcessing][ItkGolden][AdaptiveHistogramEqualizationImageFilter]")
{
  RunAheItkGolden("BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest2.png", /*alpha=*/1.0f, /*beta=*/0.25f, /*radius=*/{10, 10, 10}, /*assertBaselineGolden=*/true, /*aMaxDiffFraction=*/0.005,
                  /*bMaxDiffFraction=*/0.005);
}
