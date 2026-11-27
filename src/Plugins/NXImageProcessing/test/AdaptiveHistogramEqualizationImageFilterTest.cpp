#include "AdaptiveHistogramEqualizationFilterTestUtils.hpp"
#include "ItkGoldenTestUtils.hpp"

#include "NXImageProcessing/Filters/AdaptiveHistogramEqualizationImageFilter.hpp"

#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace aht = ahe_test;

// (2) Independent double-precision oracle on a small in-memory image. Real-disk out-of-core coverage for this
//     filter is retired to the generic SimplnxOoc store tests (the adopted model); this case keeps the UNIQUE
//     oracle-based correctness check against an independent reference. The AHE engine is single-implementation
//     (no DispatchAlgorithm, so it records no algorithm-path execution), which means it CANNOT use
//     UnitTest::AlgorithmTestScope: the scope's executeFilter witness requires a recorded path and would fail
//     with an observed count of 0. The case is therefore a plain ForceInCore in-memory correctness check.
TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: Independent oracle (in-memory)", "[NXImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_DimX = 24;
  constexpr usize k_DimY = 20;
  constexpr usize k_DimZ = 16;
  const std::vector<uint32> radius = {1, 1, 1};
  constexpr float32 k_Alpha = 0.3f;
  constexpr float32 k_Beta = 0.3f;

  DataStructure dataStructure;
  const DataPath inputPath = aht::BuildAheImage<uint8>(dataStructure, k_DimX, k_DimY, k_DimZ);

  AdaptiveHistogramEqualizationImageFilter filter;
  aht::RunAheFilter<AdaptiveHistogramEqualizationImageFilter>(filter, dataStructure, inputPath, k_Alpha, k_Beta, radius);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  std::vector<uint8> actual(outStore.getSize());
  for(usize i = 0; i < actual.size(); ++i)
  {
    actual[i] = outStore.getValue(i);
  }

  const std::vector<uint8> input = aht::MakeAhePattern<uint8>(k_DimX, k_DimY, k_DimZ);
  const std::vector<uint8> expected = aht::AdaptiveHistogramEqualizationOracle<uint8>(input, k_DimX, k_DimY, k_DimZ, {1, 1, 1}, k_Alpha, k_Beta);
  aht::RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// (3) Preflight: multi-component input is rejected (scalar-only); a valid scalar input preflights cleanly and
//     an unsupported (non-numeric) type is rejected by the shared type-membership guard.
TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: Preflight scalar/type guard", "[NXImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  AdaptiveHistogramEqualizationImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{5, 5, 5}));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(0.3f));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(0.3f));
    return args;
  };

  SECTION("scalar input preflights cleanly")
  {
    DataStructure dataStructure;
    const DataPath inputPath = aht::BuildAheImage<uint8>(dataStructure, 8, 8, 8);
    auto result = filter.preflight(dataStructure, makeArgs(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  SECTION("multi-component input is rejected")
  {
    // Build a 2-component array under the geometry and point the filter at it.
    DataStructure dataStructure;
    const DataPath scalarPath = aht::BuildAheImage<uint8>(dataStructure, 8, 8, 8); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, vecPath, {8, 8, 8}, {2});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<uint8>::Create(dataStructure, "Vec", vecStore, cellAM.getId());

    auto result = filter.preflight(dataStructure, makeArgs(vecPath));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
  }
}

// (4) FromSIMPLJson maps the legacy SIMPL keys (Radius/Alpha/Beta + array paths) to the new parameter keys.
TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: FromSIMPLJson", "[NXImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
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
std::vector<uint8> MaterializeUint8(DataStructure& dataStructure, const DataPath& path)
{
  const auto& store = dataStructure.getDataRefAs<DataArray<uint8>>(path).getDataStoreRef();
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
DataPath RunColorToGrayScale(DataStructure& dataStructure, const DataPath& inputDataPath)
{
  const Uuid kColorToGrayScaleUuid = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
  IFilter::UniquePointerType filter = Application::Instance()->getFilterList()->createFilter(kColorToGrayScaleUuid);
  REQUIRE(filter != nullptr);

  Arguments args;
  args.insertOrAssign("conversion_algorithm_index", std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign("color_weights", std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign("color_channel", std::make_any<Int32ParameterType::ValueType>(0));
  args.insertOrAssign("input_data_array_paths", std::make_any<MultiArraySelectionParameter::ValueType>({inputDataPath}));
  args.insertOrAssign("output_array_prefix", std::make_any<StringParameter::ValueType>("GrayScale_"));

  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  return inputDataPath.getParent().createChildPath(fmt::format("GrayScale_{}", inputDataPath.getTargetName()));
}

// ITK-sourced real-image golden for AHE (plan Task 6). AHE is TOLERANT: the engine accumulates a neighborhood in a
// different order than the ITK source, so the output is not bit-identical to the committed ITK baseline. The ITK
// source 2e-3 / 1e-5 CompareImages tolerance is effectively BIT-EXACT for a uint8 image, because any 1-level
// difference exceeds it. The (A) durable golden therefore compares the output to the committed .png baseline with a
// tolerant band (|delta| <= 1 per voxel on a small fraction of voxels). The test reads the input (sf4.png,
// palette/RGB) and the baseline through OUR readers. It converts the input to grayscale with the same
// ColorToGrayScale filter the ITK test uses. The whole case is ForceInCore.
void RunAheItkGolden(const std::string& baselineFile, float32 alpha, float32 beta, const std::vector<uint32>& radius, double aMaxDiffFraction)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath rgbInput = cellData.createChildPath("Input");

  // --- read sf4.png (palette/RGB) and convert to grayscale, exactly as the ITK test ---
  DataStructure dataStructure;
  const Result<> readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("sf4.png"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  const DataPath grayInput = RunColorToGrayScale(dataStructure, rgbInput);

  const auto makeArgs = [&](const std::string& outputName) {
    Arguments args;
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(grayInput));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Radius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(radius));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Alpha_Key, std::make_any<float32>(alpha));
    args.insertOrAssign(AdaptiveHistogramEqualizationImageFilter::k_Beta_Key, std::make_any<float32>(beta));
    return args;
  };

  // --- run OUR AHE on the grayscale input ---
  {
    AdaptiveHistogramEqualizationImageFilter filter;
    const Arguments args = makeArgs("Output");
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const DataPath output = cellData.createChildPath("Output");

  // --- (A) DURABLE golden: tolerant compare vs the committed .png baseline ---
  const DataPath bGeom({"Baseline Geometry"});
  const Result<> readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const std::vector<uint8> ourOut = MaterializeUint8(dataStructure, output);
  const std::vector<uint8> baselineData = MaterializeUint8(dataStructure, baseline);
  REQUIRE(ourOut.size() == baselineData.size());
  ReportDivergence(fmt::format("AHE (A) baseline='{}'", baselineFile), ourOut, baselineData, aMaxDiffFraction);
  aht::RequireAheClose<uint8>(ourOut, baselineData, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/aMaxDiffFraction);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden test (plan Task 6) -- duplicates the (histo) case of
// ITKAdaptiveHistogramEqualizationImageTest.cpp on OUR ITK-free filter. AHE is TOLERANT (not bit-reproducible).
// The tolerant band is |delta| <= 1 per voxel on <= 0.5% of voxels. The committed (histo) baseline is valid
// (measured maxAbsDiff 1), so this case asserts the durable baseline.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: ITK real-image golden (histo)", "[NXImageProcessing][ItkGolden][AdaptiveHistogramEqualizationImageFilter]")
{
  RunAheItkGolden("BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest2.png", /*alpha=*/1.0f, /*beta=*/0.25f, /*radius=*/{10, 10, 10}, /*aMaxDiffFraction=*/0.005);
}

namespace
{
template <class T>
void CheckHugeRadiusFilter(usize dimX, usize dimY, usize dimZ, const std::vector<uint32>& hugeRadius, const std::vector<uint32>& coveringRadius)
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 0);
  std::array<std::vector<T>, 2> outputs;
  const std::array<std::vector<uint32>, 2> radii = {coveringRadius, hugeRadius};
  for(usize run = 0; run < radii.size(); ++run)
  {
    DataStructure dataStructure;
    const DataPath inputPath = aht::BuildAheImage<T>(dataStructure, dimX, dimY, dimZ);
    AdaptiveHistogramEqualizationImageFilter filter;
    aht::RunAheFilter<AdaptiveHistogramEqualizationImageFilter>(filter, dataStructure, inputPath, 0.37f, 0.21f, radii[run]);
    const DataPath outputPath = inputPath.getParent().createChildPath("Output");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(outputPath));
    const auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
    REQUIRE(outputStore.getStoreType() == IDataStore::StoreType::InMemory);
    outputs[run].resize(outputStore.getSize());
    const Result<> copyResult = outputStore.copyIntoBuffer(0, nonstd::span<T>(outputs[run].data(), outputs[run].size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
  REQUIRE(outputs[0] == outputs[1]);
}
} // namespace

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: generic huge-radius clipped neighborhood", "[NXImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  // One unfixed X-only traversal can finish within the 60-second RED timeout on a fast machine.
  // Repeat the same approved fixture so that skipped-radius work reliably exceeds that process limit.
  for(usize repetition = 0; repetition < 32; ++repetition)
  {
    CAPTURE(repetition);
    CheckHugeRadiusFilter<float32>(3, 2, 1, {std::numeric_limits<uint32>::max(), 0, 0}, {2, 0, 0});
  }
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationImageFilter: LUT huge-radius clipped neighborhood", "[NXImageProcessing][AdaptiveHistogramEqualizationImageFilter]")
{
  constexpr uint32 k_Huge = std::numeric_limits<uint32>::max();
  CheckHugeRadiusFilter<uint16>(3, 3, 2, {k_Huge, k_Huge, k_Huge}, {2, 2, 1});
}
