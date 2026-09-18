#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/DiscreteGaussianImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
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
// Legacy ITKDiscreteGaussianImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the GaussianOperator FIR kernel's
// bit-exact parity gate.
const Uuid k_LegacyDiscreteGaussianUuid = *Uuid::FromString("025edc1a-986d-4005-92d1-545dfdc13abd");

using DGFilter = DiscreteGaussianImageFilter;

// Sets the standard geom/input/output keys + Variance/MaximumKernelWidth/MaximumError/UseImageSpacing on a shared
// Arguments, runs preflight + execute, and requires both succeed. Reuses DGFilter::k_*_Key for BOTH the new and the
// legacy ITK filter -- correct only because the new filter deliberately reuses the legacy key strings.
void RunDG(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const std::vector<float64>& variance, uint32 maxKernelWidth, const std::vector<float64>& maxError, bool useImageSpacing,
           const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(DGFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DGFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DGFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(DGFilter::k_Variance_Key, std::make_any<std::vector<float64>>(variance));
  args.insertOrAssign(DGFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(maxKernelWidth));
  args.insertOrAssign(DGFilter::k_MaximumError_Key, std::make_any<std::vector<float64>>(maxError));
  args.insertOrAssign(DGFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8) so
// the full 10-type parity grid can reuse it. Varies along all three axes so the smoothing is non-trivial.
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

// Run new + legacy on the same field/params and require the SameAsInput outputs match. The per-line FIR convolution is
// deterministic and thread-count-independent, so ITK matches for ALL input types: integer types compare EXACTLY (the
// per-pass static_cast<T> truncation must match ITK's Image<OutputPixelType> intermediates), floating types compare to
// EPSILON. UnitTest::CompareDataArrays<T> is exact for integer T and EPSILON for float T (3rd arg is a start offset).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, const std::vector<float64>& variance, uint32 maxKernelWidth, const std::vector<float64>& maxError, bool useImageSpacing)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  DGFilter newFilter;
  RunDG(newFilter, newDs, newInput, variance, maxKernelWidth, maxError, useImageSpacing);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunDG(*legacyFilter, legacyDs, legacyInput, variance, maxKernelWidth, maxError, useImageSpacing);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<T>(newOut, legacyOut); // exact for integer T, EPSILON for float T
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK parity, FULL type grid (all 10 scalar types) x {UseImageSpacing on/off} x {isotropic, anisotropic
//     variance} x {3D, 2D}. Output is SameAsInput; integer types compare EXACTLY, float to EPSILON. This is the primary
//     gate on the GaussianOperator FIR kernel + descending cascade + per-pass truncation.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: Live-ITK parity", "[ImageProcessing][DiscreteGaussianImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid) != nullptr);

  const bool useImageSpacing = GENERATE(false, true);
  const std::vector<float64> variance = GENERATE(std::vector<float64>{2.0, 2.0, 2.0}, std::vector<float64>{1.0, 2.0, 3.0});
  const uint32 maxKernelWidth = 32;
  const std::vector<float64> maxError{0.01, 0.01, 0.01};
  CAPTURE(useImageSpacing, variance[0], variance[1], variance[2]);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, variance, maxKernelWidth, maxError, useImageSpacing);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, variance, maxKernelWidth, maxError, useImageSpacing);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK parity with a non-unit (anisotropic) spacing geometry, UseImageSpacing ON (the variance/spacing^2 path),
//     on signed/float representative types.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][DiscreteGaussianImageFilter]", int16, int32, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  const std::vector<float64> variance{2.0, 2.0, 2.0};
  const std::vector<float64> maxError{0.01, 0.01, 0.01};

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, DX, DY, DZ, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  DGFilter newFilter;
  RunDG(newFilter, newDs, newInput, variance, 32, maxError, /*useImageSpacing=*/true);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, DX, DY, DZ, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunDG(*legacyFilter, legacyDs, legacyInput, variance, 32, maxError, /*useImageSpacing=*/true);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<T>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath)); // exact/int, EPSILON/float vs live ITK
}

// -----------------------------------------------------------------------------
// (2b) Live-ITK parity with a LARGE variance (~400) and a small MaximumKernelWidth (32), so GaussianKernelCoefficients'
//      `if(coeff.size() > maximumKernelWidth) break` clamp FIRES (the natural kernel for sigma=20 needs ~50 one-sided
//      coefficients to reach cap, far beyond 32). The primary grid only ever uses variance <= 3 (kernel well under 32),
//      so this clamp was uncovered. ITK's GaussianOperator applies the identical cap, so our filter and legacy ITK must
//      still match (EPSILON for float32).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: Live-ITK parity with MaximumKernelWidth clamp", "[ImageProcessing][DiscreteGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid) != nullptr);

  // variance 400 (sigma 20) with maxKernelWidth 32 forces the clamp; useImageSpacing false keeps variance in pixels.
  RequireParity<float32>(MakeRamp<float32>(12, 12, 12), 12, 12, 12, {400.0, 400.0, 400.0}, /*maxKernelWidth=*/32u, {0.01, 0.01, 0.01}, /*useImageSpacing=*/false);
}

// -----------------------------------------------------------------------------
// (2c) Live-ITK parity at a non-default MaximumError (0.05 instead of the 0.01 the entire primary grid pins). MaximumError
//      sets the cap = 1 - maximumError that terminates the coefficient loop, so 0.05 yields a SHORTER kernel than 0.01;
//      our filter must still match legacy ITK (which uses the same cap) -- EPSILON for float32.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: Live-ITK parity at MaximumError 0.05", "[ImageProcessing][DiscreteGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDiscreteGaussianUuid) != nullptr);

  SECTION("3D")
  {
    RequireParity<float32>(MakeRamp<float32>(12, 12, 12), 12, 12, 12, {2.0, 2.0, 2.0}, /*maxKernelWidth=*/32u, {0.05, 0.05, 0.05}, /*useImageSpacing=*/false);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<float32>(MakeRamp<float32>(20, 16, 1), 20, 16, 1, {2.0, 2.0, 2.0}, /*maxKernelWidth=*/32u, {0.05, 0.05, 0.05}, /*useImageSpacing=*/false);
  }
}

// -----------------------------------------------------------------------------
// (3) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a
//     SameAsInput output array (same element type as the input).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: preflight guards", "[ImageProcessing][DiscreteGaussianImageFilter]")
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
    DGFilter filter;
    Arguments args;
    args.insertOrAssign(DGFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(DGFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(DGFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(DGFilter::k_Variance_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(DGFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(32u));
    args.insertOrAssign(DGFilter::k_MaximumError_Key, std::make_any<std::vector<float64>>(std::vector<float64>{0.01, 0.01, 0.01}));
    args.insertOrAssign(DGFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, field);
    DGFilter filter;
    RunDG(filter, ds, inputPath, {1.0, 1.0, 1.0}, 32, {0.01, 0.01, 0.01}, /*useImageSpacing=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson: the per-axis Variance vector (legacy FloatVec3 object shape), the MaximumKernelWidth uint32, the
//     per-axis MaximumError vector, the UseImageSpacing bool, and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: FromSIMPLJson", "[ImageProcessing][DiscreteGaussianImageFilter]")
{
  const nlohmann::json json = {
      {"Variance", {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}}},
      {"MaximumKernelWidth", 40},
      {"MaximumError", {{"x", 0.02}, {"y", 0.03}, {"z", 0.04}}},
      {"UseImageSpacing", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "DGOut"},
  };
  const Result<Arguments> result = DiscreteGaussianImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<VectorFloat64Parameter::ValueType>(DGFilter::k_Variance_Key) == std::vector<float64>{2.0, 3.0, 4.0});
  REQUIRE(args.value<uint32>(DGFilter::k_MaximumKernelWidth_Key) == 40u);
  REQUIRE(args.value<VectorFloat64Parameter::ValueType>(DGFilter::k_MaximumError_Key) == std::vector<float64>{0.02, 0.03, 0.04});
  REQUIRE(args.value<bool>(DGFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(DGFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(DGFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(DGFilter::k_OutputImageArrayName_Key) == "DGOut");
}

namespace
{
// Shared body for the two DiscreteGaussian BASELINE golden cases (float + short). DiscreteGaussian is SameAsInput, so
// its output element type equals the input's (float32 for RA-Float.nrrd, int16 for RA-Slice-Short.nrrd) -- the generic
// rt::RunDistanceMapItkGoldenBaseline helper is float32-only, so this templated variant is used instead. Mirrors
// ITKDiscreteGaussianImageTest.cpp: read the ITK input through OUR reader, run OUR filter, then --
//   (B) LIVE-ITK parity: legacy ITK filter on the SAME input, compared via UnitTest::CompareDataArrays<T> (EXACT for
//       integer T, EPSILON for float T -- DiscreteGaussian's established parity class, matching the synthetic tests);
//   (A) DURABLE golden: compare our output to ITK's committed baseline at the ITK case's tolerance.
// Pinned ForceInCore (the legacy ITK filter bad_casts an OOC store).
template <class T>
void RunDiscreteGaussianBaselineGolden(const std::string& inputFile, const std::string& baselineFile, float64 tolerance, const rt::ExtraParamSetter& setExtraParams = rt::NoExtraParams())
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // NOTE: every side-effecting Result is bound to a NAMED local before SIMPLNX_RESULT_REQUIRE_VALID, because that macro
  // evaluates its argument more than once -- inlining a call that mutates `ds` (a read/execute) would run it twice and
  // the second run would fail (geometry/array already exists). This is the required local-binding idiom.
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(DiscreteGaussianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(DiscreteGaussianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(DiscreteGaussianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setExtraParams(args);
  DiscreteGaussianImageFilter filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (B) LIVE-ITK parity into the SAME ds (distinct output name).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<DiscreteGaussianImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(DiscreteGaussianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const Result<> itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = cellData.createChildPath("ITK Output");
  const auto& ourOut = ds.getDataRefAs<IDataArray>(output);
  const auto& itkOut = ds.getDataRefAs<IDataArray>(itkOutput);
  REQUIRE(ourOut.getDataType() == itkOut.getDataType());
  UnitTest::CompareDataArrays<T>(ourOut, itkOut); // exact int / EPSILON float vs live ITK

  // (A) DURABLE baseline golden.
  const DataPath bGeom({"Baseline Geometry"});
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- DiscreteGaussian is the MIXED filter (2 baseline cases + 1 md5 case), all three
// duplicating ITKDiscreteGaussianImageTest.cpp on OUR ITK-free filter.
//
// (float) RA-Float.nrrd -> float32 output; (A) baseline @0.0001, (B) live-ITK EPSILON-tolerant (float parity class).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  RunDiscreteGaussianBaselineGolden<float32>("RA-Float.nrrd", "BasicFilters_DiscreteGaussianImageFilter_float.nrrd", /*tolerance=*/0.0001);
}

// -----------------------------------------------------------------------------
// (short) RA-Slice-Short.nrrd -> int16 output; (A) baseline @1.0 (the ITK test's own documented tolerant tolerance for
// this integer case), (B) live-ITK EXACT (integer parity class).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  RunDiscreteGaussianBaselineGolden<int16>("RA-Slice-Short.nrrd", "BasicFilters_DiscreteGaussianImageFilter_short.nrrd", /*tolerance=*/1.0);
}

// -----------------------------------------------------------------------------
// (bigG) WhiteDots.png -> uint8 output; (A) md5-validity-first golden (plan Sec.4), (B) live-ITK BIT-EXACT (uint8
// integer parity, so the md5 helper's CompareImages@0.0 gate is the correct (B) comparator). Non-default params:
// Variance {100,100,100}, MaximumKernelWidth 64.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (bigG)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<DiscreteGaussianImageFilter>("WhiteDots.png", "f2f002ec76313284a4cff24c3e5eb577", [](Arguments& args) {
    args.insertOrAssign(DiscreteGaussianImageFilter::k_Variance_Key, std::make_any<VectorFloat64Parameter::ValueType>(VectorFloat64Parameter::ValueType{100.0, 100.0, 100.0}));
    args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(64u));
  });
}
