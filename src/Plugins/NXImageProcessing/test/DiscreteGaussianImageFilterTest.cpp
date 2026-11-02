#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/DiscreteGaussianImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using DGFilter = DiscreteGaussianImageFilter;

// Sets the standard geom/input/output keys + Variance/MaximumKernelWidth/MaximumError/UseImageSpacing on a shared
// Arguments, runs preflight + execute, and requires both succeed.
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

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8).
// It varies along all three axes, so the smoothing is non-trivial.
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
} // namespace

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
// rt::RunDistanceMapItkGoldenBaseline helper is float32-only, so this variant is used instead. Mirrors
// ITKDiscreteGaussianImageTest.cpp: read the ITK input through OUR reader, run OUR filter, then --
//   (A) DURABLE golden: compare our output to ITK's committed baseline at the ITK case's tolerance.
// Pinned ForceInCore.
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
// (float) RA-Float.nrrd -> float32 output; (A) baseline @0.0001.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  RunDiscreteGaussianBaselineGolden("RA-Float.nrrd", "BasicFilters_DiscreteGaussianImageFilter_float.nrrd", /*tolerance=*/0.0001);
}

// -----------------------------------------------------------------------------
// (short) RA-Slice-Short.nrrd -> int16 output; (A) baseline @1.0 (the ITK test's own documented tolerant tolerance for
// this integer case).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  RunDiscreteGaussianBaselineGolden("RA-Slice-Short.nrrd", "BasicFilters_DiscreteGaussianImageFilter_short.nrrd", /*tolerance=*/1.0);
}

// -----------------------------------------------------------------------------
// (bigG) WhiteDots.png -> uint8 output; (A) md5-validity-first golden (plan Sec.4). Non-default params:
// Variance {100,100,100}, MaximumKernelWidth 64.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DiscreteGaussianImageFilter: ITK real-image golden (bigG)", "[ImageProcessing][ItkGolden][DiscreteGaussianImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<DiscreteGaussianImageFilter>("WhiteDots.png", "f2f002ec76313284a4cff24c3e5eb577", [](Arguments& args) {
    args.insertOrAssign(DiscreteGaussianImageFilter::k_Variance_Key, std::make_any<VectorFloat64Parameter::ValueType>(VectorFloat64Parameter::ValueType{100.0, 100.0, 100.0}));
    args.insertOrAssign(DiscreteGaussianImageFilter::k_MaximumKernelWidth_Key, std::make_any<uint32>(64u));
  });
}
