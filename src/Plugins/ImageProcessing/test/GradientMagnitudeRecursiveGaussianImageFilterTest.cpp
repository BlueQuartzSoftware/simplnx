#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GradientMagnitudeRecursiveGaussianImageFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using GradMagFilter = GradientMagnitudeRecursiveGaussianImageFilter;

// Sets the geometry, input, and output keys plus the scalar sigma and the normalize flag on the Arguments. The helper
// runs preflight and execute, and requires that both steps succeed.
void RunGradMag(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 sigma, bool normalize, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(sigma));
  args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(normalize));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
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
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight rejections: a too-small image (fewer than 4 pixels along a filtered axis) and a non-positive sigma.
//     All scalar types are allowed, so there is no unsigned-rejection case.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeRecursiveGaussianImageFilter: preflight rejections", "[ImageProcessing][GradientMagnitudeRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("too-few-pixels rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(3 * 8 * 8, int16{1}); // X = 3 < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 3, 8, 8, field);
    GradMagFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
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
    GradMagFilter filter;
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("3D image with Z==3 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 3, int16{1}); // Z == 3 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 3, field);
    GradMagFilter filter;
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("Z==1 accepted as a 2D image")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 1, int16{1}); // Z == 1 -> treated as 2D, so Z is not required to be >= 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, field);
    GradMagFilter filter;
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  SECTION("non-positive sigma rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims + type; only sigma is invalid
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    GradMagFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
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
    GradMagFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_Sigma_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(GradMagFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianNonPositiveSpacing);
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeRecursiveGaussianImageFilter: fused plane path matches direct path", "[ImageProcessing][GradientMagnitudeRecursiveGaussianImageFilter]")
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
    GradMagFilter filter;
    RunGradMag(filter, directDs, directInput, 1.75, true);
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
    GradMagFilter filter;
    RunGradMag(filter, fusedDs, fusedInput, 1.75, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnInMemoryStore == 1);
    REQUIRE(counts.InCore == 0);
  }

  rt::RequireExactFloat32(directDs.getDataRefAs<IDataArray>(outputPath), fusedDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (2) FromSIMPLJson: the scalar Sigma double, the NormalizeAcrossScale bool, and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeRecursiveGaussianImageFilter: FromSIMPLJson", "[ImageProcessing][GradientMagnitudeRecursiveGaussianImageFilter]")
{
  const nlohmann::json json = {
      {"Sigma", 2.5},
      {"NormalizeAcrossScale", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "GradMagOut"},
  };
  const Result<Arguments> result = GradientMagnitudeRecursiveGaussianImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(GradMagFilter::k_Sigma_Key) == 2.5);
  REQUIRE(args.value<bool>(GradMagFilter::k_NormalizeAcrossScale_Key) == true);
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(GradMagFilter::k_OutputImageArrayName_Key) == "GradMagOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- "default" case. Duplicates ITKGradientMagnitudeRecursiveGaussianImageTest.cpp on
// our ITK-free filter: the test reads RA-Float.nrrd (float32) and runs our filter with the ITK case's parameters (all
// defaults; Sigma 1.0, NormalizeAcrossScale false). The durable golden compares the output with the committed baseline
// at a tolerance of 0.0001 via ip_golden::CompareImages. The output is a fixed float32 (AlwaysFloat32). The helper pins
// ForceInCore.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeRecursiveGaussianImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][GradientMagnitudeRecursiveGaussianImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientMagnitudeRecursiveGaussianImageFilter>("RA-Float.nrrd", "BasicFilters_GradientMagnitudeRecursiveGaussianImageFilter_default.nrrd", /*tolerance=*/0.0001);
}
