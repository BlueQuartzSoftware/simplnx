#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CurvatureAnisotropicDiffusionImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
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
using CurvatureAnisoFilter = CurvatureAnisotropicDiffusionImageFilter;

// Sets the standard geom/input/output keys + TimeStep/ConductanceParameter/ConductanceScalingUpdateInterval/
// NumberOfIterations on a shared Arguments, runs preflight + execute, and requires both succeed.
void RunCurvatureAniso(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, float64 conductanceParameter, uint32 conductanceScalingUpdateInterval,
                       uint32 numberOfIterations, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(conductanceParameter));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(conductanceScalingUpdateInterval));
  args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], varying along all three axes so the gradient (and
// the conductance term it feeds) is non-trivial.
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
// (3) Preflight guards: an INTEGER input is rejected (FloatingScalar admits only float32/float64, unlike
//     CurvatureFlow's AllNumeric); a multi-component input is rejected (requireScalar); a valid float input
//     produces a SameAsInput output array; and a TimeStep exceeding the CFL stability bound is VALID but carries a
//     warning (matching ITK's own non-fatal InitializeIteration check).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: preflight guards", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("integer input rejected")
  {
    // Rejected by the ArraySelectionParameter's own allowed-type check (GetFloatingScalarTypes() in parameters())
    // BEFORE preflightImpl/PreflightImageFilter ever runs -- so this asserts rejection, not a specific error code
    // (unlike the multi-component case below, which int32 with 1 component sails past that parameter check and is
    // rejected inside PreflightImageFilter itself, where the code IS stable).
    DataStructure ds;
    const std::vector<int32> field = MakeRamp<int32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, field);
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 0.0f));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<float32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<float32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar float input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    CurvatureAnisoFilter filter;
    RunCurvatureAniso(filter, ds, inputPath, 0.0625, 3.0, 1u, 5u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
  SECTION("over-large TimeStep is VALID but carries a stability warning")
  {
    // effDim==2 (Z==1), unit spacing -> stable bound = 1/2^3 = 0.125. 0.5 is well above that.
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    REQUIRE_FALSE(result.outputActions.warnings().empty());
    REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  }
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image golden cases below. Mirrors
// ITKImageProcessing/test/ITKCurvatureAnisotropicDiffusionImageTest.cpp exactly: read RA-Float.nrrd (float32) through
// OUR ITK-free reader, run OUR filter with the ITK case's params (defaults, or defaults + NumberOfIterations=10 for
// the "longer" case), then apply the plan's two golden oracles:
//   (A) DURABLE golden -- compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at the
//       ITK test's tolerance 0.1 via ip_golden::CompareImages (permanent, survives ITK removal).
// The whole case is pinned ForceInCore. All unset params (ConductanceParameter, ConductanceScalingUpdateInterval,
// and -- for the defaults case -- NumberOfIterations) resolve to the filter's defaults.
// -----------------------------------------------------------------------------
void RunCurvatureAnisoItkGolden(float64 timeStep, std::optional<uint32> numberOfIterations, const std::string& baselineFile)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR ITK-free reader ---
  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- run OUR filter with the ITK case's params ---
  Arguments args;
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  if(numberOfIterations.has_value())
  {
    args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(*numberOfIterations));
  }
  CurvatureAnisotropicDiffusionImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: compare to ITK's committed baseline at tol 0.1 (SameAsInput output is owned by 'geom') ---
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.1);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

// -----------------------------------------------------------------------------
// (5) ITK-sourced real-image golden -- "defaults" case: RA-Float.nrrd, TimeStep 0.01, all else default. Duplicates
//     ITKCurvatureAnisotropicDiffusionImageTest.cpp(defaults) on OUR ITK-free filter: (A) baseline @0.1.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][CurvatureAnisotropicDiffusionImageFilter]")
{
  RunCurvatureAnisoItkGolden(/*timeStep=*/0.01, /*numberOfIterations=*/std::nullopt, "BasicFilters_CurvatureAnisotropicDiffusionImageFilter_defaults.nrrd");
}

// -----------------------------------------------------------------------------
// (6) ITK-sourced real-image golden -- "longer" case: RA-Float.nrrd, TimeStep 0.01, NumberOfIterations 10. Duplicates
//     ITKCurvatureAnisotropicDiffusionImageTest.cpp(longer).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][CurvatureAnisotropicDiffusionImageFilter]")
{
  RunCurvatureAnisoItkGolden(/*timeStep=*/0.01, /*numberOfIterations=*/10u, "BasicFilters_CurvatureAnisotropicDiffusionImageFilter_longer.nrrd");
}

// -----------------------------------------------------------------------------
// (7) A spatially-constant (uniform) image is an IDENTITY: its average squared gradient magnitude is 0, so the global
//     reduction sets m_K = 0, and CurvatureAnisoFn's `if(m_K != 0.0)` conductance branch is skipped (Cx == Cxd == 0);
//     speed is 0 at every voxel, propagationGradient is 0, the returned update is 0, and accum never changes ->
//     output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: constant image is identity", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 7.0f));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}

// -----------------------------------------------------------------------------
// (8) The CFL stability WARNING for a genuinely 3D image (effDim == 3): the existing preflight-guards test only trips
//     the effDim==2 (Z==1) arm, so the ImageDimension==3 stable-bound branch of AppendUnstableTimeStepWarning was never
//     asserted. A 6x6x6 image at unit spacing has stable bound 1/2^(3+1) = 0.0625; TimeStep 0.5 is well above it, so
//     preflight stays VALID but carries the shared unstable-time-step warning, whose message names ImageDimension=3.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: over-large TimeStep warns (3D, ImageDimension=3)", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 6;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  Arguments args;
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
  args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
  const auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE_FALSE(result.outputActions.warnings().empty());
  REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  REQUIRE(result.outputActions.warnings().front().message.find("ImageDimension=3") != std::string::npos);
}

// -----------------------------------------------------------------------------
// (9) ConductanceScalingUpdateInterval == 0 is legal and must run cleanly (no modulo-by-zero). UpdateGlobalConductance
//     guards the cadence test with `fn.interval != 0 && (iter % fn.interval) != 0`, so interval 0 is treated as "update
//     every iteration" rather than performing `iter % 0`. RunCurvatureAniso REQUIREs both preflight and execute VALID.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: zero conductance update interval runs cleanly", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/0u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
}

// -----------------------------------------------------------------------------
// (10) NumberOfIterations == 0 is a no-op: the driver loop never runs, so the output = cast<T>(accum = cast<Real>(input))
//      is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: zero iterations is a no-op", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/0u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
