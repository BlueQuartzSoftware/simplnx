#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LaplacianRecursiveGaussianImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKLaplacianRecursiveGaussianImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the shared
// Deriche engine's second-derivative parity gate.
const Uuid k_LegacyLaplacianUuid = *Uuid::FromString("782d76a4-e3f6-4c2a-a1b0-7456a3e77f24");

using LaplacianFilter = LaplacianRecursiveGaussianImageFilter;

// Sets the standard geom/input/output keys + the scalar sigma + normalize flag on a shared Arguments, runs preflight +
// execute, and requires both succeed. Reuses LaplacianFilter::k_*_Key for BOTH the new and the legacy ITK filter --
// correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "sigma", "normalize_across_scale").
void RunLaplacian(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 sigma, bool normalize, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(sigma));
  args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(normalize));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8) so
// the full 10-type parity grid can reuse it. Varies along all three axes so the Laplacian is non-trivial.
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

// Run new + legacy on the same field/params and require the float32 Laplacian outputs match. The per-line IIR is
// deterministic (one thread fully processes each independent line) and the composites are per-voxel functors, so ITK is
// thread-count-independent here -- bit-exact vs live ITK is expected for ALL input types, float32 included
// (Parity model #4). Uses rt::RequireExactFloat32 for every type.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 sigma, bool normalize)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  LaplacianFilter newFilter;
  RunLaplacian(newFilter, newDs, newInput, sigma, normalize);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyLaplacianUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunLaplacian(*legacyFilter, legacyDs, legacyInput, sigma, normalize);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  rt::RequireExactFloat32(newOut, legacyOut); // bit-exact vs live ITK for every input type (deterministic composite)
}
} // namespace

TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: resident state requires a complete dataset-scaled reservation",
          "[ImageProcessing][LaplacianRecursiveGaussianImageFilter][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr usize valueCount = dimX * dimY * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto float32Result = ImageProcessing::detail::CalculateLaplacianRecursiveGaussianResidentWorkingMemoryBytes<float32>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(float32Result);
  REQUIRE(float32Result.value() == valueCount * (sizeof(float32) + 2 * sizeof(float32)));
  REQUIRE(float32Result.value() == 384 * k_MiB);
  auto float64Result = ImageProcessing::detail::CalculateLaplacianRecursiveGaussianResidentWorkingMemoryBytes<float64>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(float64Result);
  REQUIRE(float64Result.value() == 512 * k_MiB);
  auto overflowResult = ImageProcessing::detail::CalculateLaplacianRecursiveGaussianResidentWorkingMemoryBytes<float64>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  REQUIRE(ImageProcessing::detail::ShouldUseLaplacianRecursiveGaussianResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseLaplacianRecursiveGaussianResidentState(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(1024 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveLaplacianRecursiveGaussianResidentWorkingMemory<float32>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 256 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(2 * 1024 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveLaplacianRecursiveGaussianResidentWorkingMemory<float32>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == float32Result.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL type grid (all 10 scalar types incl. float) x {normalize on/off} x {3D, 2D}. The
//     Laplacian is a fixed float32 output; the new filter must reproduce legacy ITK EXACTLY (no tolerance). This is a
//     primary gate on the shared Deriche recursive-Gaussian engine (order-2 second-derivative path).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: Live-ITK parity", "[ImageProcessing][LaplacianRecursiveGaussianImageFilter]", uint8, int8, uint16, int16, uint32, int32,
                   uint64, int64, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyLaplacianUuid) != nullptr);

  const bool normalize = GENERATE(false, true);
  CAPTURE(normalize);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, 2.0, normalize);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, 1.5, normalize);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry (the spacing math path), on signed/float
//     representative types.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][LaplacianRecursiveGaussianImageFilter]", int16, int32, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyLaplacianUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, DX, DY, DZ, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  LaplacianFilter newFilter;
  RunLaplacian(newFilter, newDs, newInput, 1.5, false);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyLaplacianUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, DX, DY, DZ, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunLaplacian(*legacyFilter, legacyDs, legacyInput, 1.5, false);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExactFloat32(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath)); // bit-exact vs live ITK
}

// -----------------------------------------------------------------------------
// (3) Preflight rejections: a too-small image (fewer than 4 pixels along a filtered axis) and a non-positive sigma.
//     All scalar types are allowed, so there is no unsigned-rejection case.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: preflight rejections", "[ImageProcessing][LaplacianRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("too-few-pixels rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(3 * 8 * 8, int16{1}); // X = 3 < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 3, 8, 8, field);
    LaplacianFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  // The "too-few-pixels" arm above trips via X == 3. ValidateSeparableImageDims separately rejects a genuinely 3D image
  // (Z > 1) whose Z < 4 (`zIs2D || dims[2] >= 4`, zIs2D == (dims[2]==1)); a Z of 2 or 3 (NOT treated as 2D) hits that
  // arm, while Z == 1 is accepted as a 2D image. All other params here are valid so the dims check is the only failure.
  SECTION("3D image with Z==2 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 2, int16{1}); // Z == 2 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 2, field);
    LaplacianFilter filter;
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("3D image with Z==3 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 3, int16{1}); // Z == 3 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 3, field);
    LaplacianFilter filter;
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("Z==1 accepted as a 2D image")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 1, int16{1}); // Z == 1 -> treated as 2D, so Z is not required to be >= 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, field);
    LaplacianFilter filter;
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  SECTION("non-positive sigma rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims + type; only sigma is invalid
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    LaplacianFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianNonPositiveSigma);
  }
  SECTION("degenerate spacing rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims/type/sigma; only the X spacing is degenerate
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    LaplacianFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(LaplacianFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(LaplacianFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LaplacianFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LaplacianFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(LaplacianFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianNonPositiveSpacing);
  }
}

// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: fused plane path matches direct path", "[ImageProcessing][LaplacianRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(1ULL, 8ULL);
  const FloatVec3 spacing{0.6f, 1.4f, 2.2f};
  const std::vector<float64> field = MakeRamp<float64>(12, 11, dimZ);
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  DataStructure directDs;
  const DataPath directInput = rt::BuildImageFromPattern<float64>(directDs, 12, 11, dimZ, field);
  directDs.getDataRefAs<ImageGeom>(directInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceInCoreAlgorithmGuard guard;
    ResetAlgorithmPathExecutionCounts();
    LaplacianFilter filter;
    RunLaplacian(filter, directDs, directInput, 1.75, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.InCoreOnInMemoryStore == 1);
    REQUIRE(counts.OutOfCore == 0);
  }

  DataStructure fusedDs;
  const DataPath fusedInput = rt::BuildImageFromPattern<float64>(fusedDs, 12, 11, dimZ, field);
  fusedDs.getDataRefAs<ImageGeom>(fusedInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceOocAlgorithmGuard guard(true);
    ResetAlgorithmPathExecutionCounts();
    LaplacianFilter filter;
    RunLaplacian(filter, fusedDs, fusedInput, 1.75, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnInMemoryStore == 1);
    REQUIRE(counts.InCore == 0);
  }

  rt::RequireExactFloat32(directDs.getDataRefAs<IDataArray>(outputPath), fusedDs.getDataRefAs<IDataArray>(outputPath));
}

// (5) FromSIMPLJson: the scalar Sigma double, the NormalizeAcrossScale bool, and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: FromSIMPLJson", "[ImageProcessing][LaplacianRecursiveGaussianImageFilter]")
{
  const nlohmann::json json = {
      {"Sigma", 2.5},
      {"NormalizeAcrossScale", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "LaplacianOut"},
  };
  const Result<Arguments> result = LaplacianRecursiveGaussianImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(LaplacianFilter::k_Sigma_Key) == 2.5);
  REQUIRE(args.value<bool>(LaplacianFilter::k_NormalizeAcrossScale_Key) == true);
  REQUIRE(args.value<DataPath>(LaplacianFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(LaplacianFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(LaplacianFilter::k_OutputImageArrayName_Key) == "LaplacianOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- "default" case. Duplicates ITKLaplacianRecursiveGaussianImageTest.cpp on OUR
// ITK-free filter: read RA-Float.nrrd (float32), run OUR filter with the ITK case's params (all defaults; Sigma 1.0,
// NormalizeAcrossScale false in both filters), then apply the plan's two oracles --
//   (A) DURABLE golden: compare to the committed baseline at tol 0.0001 via ip_golden::CompareImages;
//   (B) LIVE-ITK parity: legacy ITK filter on the SAME input, BIT-EXACT (established parity class, rt::RequireExactFloat32).
// Output is a fixed float32 (AlwaysFloat32). Helper pins ForceInCore.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LaplacianRecursiveGaussianImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][LaplacianRecursiveGaussianImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<LaplacianRecursiveGaussianImageFilter>("RA-Float.nrrd", "BasicFilters_LaplacianRecursiveGaussianImageFilter_default.nrrd", /*tolerance=*/0.0001,
                                                                             /*bitExactB=*/true);
}
