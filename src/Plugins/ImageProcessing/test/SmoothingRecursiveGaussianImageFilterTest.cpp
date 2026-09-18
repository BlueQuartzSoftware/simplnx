#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SmoothingRecursiveGaussianImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using SmoothFilter = SmoothingRecursiveGaussianImageFilter;
// NOTE: the legacy ITKSmoothingRecursiveGaussianImageFilter is DISABLED in the ITKImageProcessing build, so there is
// no live-ITK reference for this filter (see Parity model #5). Its Deriche engine is gated bit-exactly against live
// ITK by the GradientMagnitude/Laplacian parity tests (the same RecursiveGaussianAxisPass). Here: analytic + D3.

// A centrally-symmetric radial blob (for the symmetry-preservation check).
template <class T>
std::vector<T> MakeRadial(usize dx, usize dy, usize dz)
{
  std::vector<T> f(dx * dy * dz);
  const double cx = (dx - 1) / 2.0;
  const double cy = (dy - 1) / 2.0;
  const double cz = (dz - 1) / 2.0;
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        const double r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz);
        f[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>(100.0 * std::exp(-r2 / 8.0));
      }
    }
  }
  return f;
}

template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> f(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        f[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((3 * x + 2 * y + z) % 17);
      }
    }
  }
  return f;
}

void RunSmooth(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const std::vector<float64>& sigma, bool normalize, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(sigma));
  args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(normalize));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

template <class T>
std::vector<T> ReadArray(const DataStructure& ds, const DataPath& path)
{
  const auto& store = ds.getDataRefAs<DataArray<T>>(path).getDataStoreRef();
  std::vector<T> out(store.getSize());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = store.getValue(i);
  }
  return out;
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Smoothing preserves a constant image (all signed types). A normalized (DC-gain-1) Gaussian of a constant is the
//     constant, incl. at the edge-extended boundary. Margin 1.0 tolerates the final float32->int static_cast truncation.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: constant image preserved", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]", int8, int16, int32, int64, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 12;
  const std::vector<T> field(d * d * d, static_cast<T>(20));
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<T>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
  const std::vector<T> out = ReadArray<T>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  for(T v : out)
  {
    REQUIRE(std::abs(static_cast<double>(v) - 20.0) <= 1.0);
  }
}

// -----------------------------------------------------------------------------
// (2) Smoothing preserves central symmetry (float64 radial blob) and reduces the peak (variance reduction).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: symmetric blob stays symmetric", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 13; // odd so there is a true center voxel
  const std::vector<float64> field = MakeRadial<float64>(d, d, d);
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));

  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  const usize c = d / 2;
  for(usize y = 0; y < d; ++y)
  {
    for(usize x = 0; x < d; ++x)
    {
      REQUIRE(at(x, y, c) == Approx(at(d - 1 - x, y, c)).margin(1e-3)); // mirror across X (float32 intermediates -> ~1e-3)
      REQUIRE(at(x, y, c) == Approx(at(x, d - 1 - y, c)).margin(1e-3)); // mirror across Y
    }
  }
  REQUIRE(at(c, c, c) < 100.0); // peak reduced by smoothing
  REQUIRE(at(c, c, c) > 0.0);
}

// -----------------------------------------------------------------------------
// (4) Preflight rejects unsigned input (SignedScalar policy) and too-small dims.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: preflight rejections", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("unsigned input rejected")
  {
    DataStructure ds;
    const std::vector<uint16> field(8 * 8 * 8, uint16{3});
    const DataPath inputPath = rt::BuildImageFromPattern<uint16>(ds, 8, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("too-few-pixels rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(3 * 8 * 8, int16{1}); // X = 3 < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 3, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
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
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("3D image with Z==3 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 3, int16{1}); // Z == 3 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 3, field);
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("Z==1 accepted as a 2D image")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 1, int16{1}); // Z == 1 -> treated as 2D, so Z is not required to be >= 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, field);
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  SECTION("non-positive sigma rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims + signed type; only sigma is invalid
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{0.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
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
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianNonPositiveSpacing);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson: the per-axis Sigma vector (legacy FloatVec3 object shape), the NormalizeAcrossScale bool, and the
//     geometry/array/name DataPaths. Sigma uses DoubleVec3FilterParameterConverter -> std::vector<float64>.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: FromSIMPLJson", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  const nlohmann::json json = {
      {"Sigma", {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}}},
      {"NormalizeAcrossScale", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "SmoothOut"},
  };
  const Result<Arguments> result = SmoothingRecursiveGaussianImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<VectorFloat64Parameter::ValueType>(SmoothFilter::k_Sigma_Key) == std::vector<float64>{2.0, 3.0, 4.0});
  REQUIRE(args.value<bool>(SmoothFilter::k_NormalizeAcrossScale_Key) == true);
  REQUIRE(args.value<DataPath>(SmoothFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(SmoothFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(SmoothFilter::k_OutputImageArrayName_Key) == "SmoothOut");
}

// -----------------------------------------------------------------------------
// (6) Anisotropic per-axis sigma: a larger sigma on an axis spreads a central impulse MORE along that axis. This guards
//     the per-axis sigma[axis] indexing in SmoothingRecursiveGaussianExecuteFn's custom "last-axis-first" cascade order
//     -- a symmetric-sigma test cannot catch a sigma/axis swap. Smoothing has no live-ITK oracle, so this is an
//     analytic directional check (the Gz(0) factor cancels, so the X-vs-Y comparison reduces to exp(2/sy^2 - 2/sx^2)).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: anisotropic sigma spreads along the larger-sigma axis", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 15;
  std::vector<float64> field(d * d * d, 0.0);
  const usize c = d / 2;
  field[(c * d + c) * d + c] = 1.0; // central impulse
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {3.0, 1.0, 1.0}, false); // strong blur along X, weak along Y/Z
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  REQUIRE(at(c + 2, c, c) > at(c, c + 2, c));                       // larger sigma_x -> more spread along X than Y
  REQUIRE(at(c, c + 2, c) == Approx(at(c, c, c + 2)).margin(1e-6)); // sigma_y == sigma_z
  REQUIRE(at(c + 2, c, c) == Approx(at(c - 2, c, c)).margin(1e-6)); // symmetric along X
}

// -----------------------------------------------------------------------------
// (7) Anisotropic spacing: a larger spacing on an axis shrinks that axis's voxel-domain sigma (sigmad = sigma/spacing),
//     so the impulse spreads LESS along the larger-spacing axis. Guards the per-axis spacing[axis] indexing.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: anisotropic spacing scales the per-axis blur", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 15;
  std::vector<float64> field(d * d * d, 0.0);
  const usize c = d / 2;
  field[(c * d + c) * d + c] = 1.0;
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  ds.getDataRefAs<ImageGeom>(input.getParent().getParent()).setSpacing(FloatVec3{2.0f, 1.0f, 1.0f}); // coarser X spacing
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false); // isotropic physical sigma -> sigmad_x=1 < sigmad_y=sigmad_z=2
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  REQUIRE(at(c + 2, c, c) < at(c, c + 2, c));                       // coarser X spacing -> smaller sigmad_x -> less spread along X
  REQUIRE(at(c, c + 2, c) == Approx(at(c, c, c + 2)).margin(1e-6)); // Y and Z share spacing + sigma
}

// -----------------------------------------------------------------------------
// (8) 2D (Z==1) smoothing exercises the effDim==2 cascade path in SmoothingRecursiveGaussianExecuteFn: a constant is
//     preserved and a symmetric blob stays symmetric.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: 2D smoothing preserves a constant and stays symmetric", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 13;
  SECTION("constant preserved")
  {
    const std::vector<float64> field(d * d * 1, 20.0);
    DataStructure ds;
    const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, 1, field);
    SmoothFilter filter;
    RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
    const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
    for(float64 v : out)
    {
      REQUIRE(v == Approx(20.0).margin(1e-4));
    }
  }
  SECTION("symmetric blob stays symmetric")
  {
    const std::vector<float64> field = MakeRadial<float64>(d, d, 1);
    DataStructure ds;
    const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, 1, field);
    SmoothFilter filter;
    RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
    const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
    auto at = [&](usize x, usize y) { return out[y * d + x]; };
    for(usize y = 0; y < d; ++y)
    {
      for(usize x = 0; x < d; ++x)
      {
        REQUIRE(at(x, y) == Approx(at(d - 1 - x, y)).margin(1e-3)); // mirror across X
        REQUIRE(at(x, y) == Approx(at(x, d - 1 - y)).margin(1e-3)); // mirror across Y
      }
    }
  }
}

// -----------------------------------------------------------------------------
// (9) NormalizeAcrossScale is a no-op for order-0 smoothing (ITK applies it only to derivative orders 1/2). Pin the
//     documented invariant: normalize on/off produce byte-identical output.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: NormalizeAcrossScale is a no-op for smoothing", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 12;
  const std::vector<float64> field = MakeRadial<float64>(d, d, d);
  DataStructure dsOff;
  const DataPath inOff = rt::BuildImageFromPattern<float64>(dsOff, d, d, d, field);
  SmoothFilter fOff;
  RunSmooth(fOff, dsOff, inOff, {2.0, 2.0, 2.0}, false);
  DataStructure dsOn;
  const DataPath inOn = rt::BuildImageFromPattern<float64>(dsOn, d, d, d, field);
  SmoothFilter fOn;
  RunSmooth(fOn, dsOn, inOn, {2.0, 2.0, 2.0}, true);
  const std::vector<float64> a = ReadArray<float64>(dsOff, DataPath({"Image Geometry", "CellData", "Output"}));
  const std::vector<float64> b = ReadArray<float64>(dsOn, DataPath({"Image Geometry", "CellData", "Output"}));
  REQUIRE(a == b); // byte-identical: order-0 ignores normalizeAcrossScale
}

// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: fused plane path matches direct path", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]", int16, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(1ULL, 8ULL);
  const FloatVec3 spacing{0.6f, 1.4f, 2.2f};
  const std::vector<T> field = MakeRamp<T>(12, 11, dimZ);
  const std::vector<float64> sigma{1.25, 2.0, 1.6};
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  DataStructure directDs;
  const DataPath directInput = rt::BuildImageFromPattern<T>(directDs, 12, 11, dimZ, field);
  directDs.getDataRefAs<ImageGeom>(directInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceInCoreAlgorithmGuard guard;
    ResetAlgorithmPathExecutionCounts();
    SmoothFilter filter;
    RunSmooth(filter, directDs, directInput, sigma, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.InCoreOnInMemoryStore == 1);
    REQUIRE(counts.OutOfCore == 0);
  }

  DataStructure fusedDs;
  const DataPath fusedInput = rt::BuildImageFromPattern<T>(fusedDs, 12, 11, dimZ, field);
  fusedDs.getDataRefAs<ImageGeom>(fusedInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceOocAlgorithmGuard guard(true);
    ResetAlgorithmPathExecutionCounts();
    SmoothFilter filter;
    RunSmooth(filter, fusedDs, fusedInput, sigma, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnInMemoryStore == 1);
    REQUIRE(counts.InCore == 0);
  }

  REQUIRE(ReadArray<T>(directDs, outputPath) == ReadArray<T>(fusedDs, outputPath));
}

// ITK-sourced real-image golden -- "default" case. Duplicates ITKSmoothingRecursiveGaussianImageTest.cpp(default) on
// OUR ITK-free filter: read RA-Float.nrrd (float32), run OUR filter with the ITK case's params (all defaults; per-axis
// Sigma {1,1,1}, NormalizeAcrossScale false), then compare to ITK's committed baseline at tol 0.0001.
//
// RECORDED DIVERGENCE (disabled-legacy): this is an (A)-DURABLE-GOLDEN-ONLY case. The legacy
// ITKSmoothingRecursiveGaussianImageFilter is DISABLED (commented out) in the ITKImageProcessing build
// (ITKImageProcessing/CMakeLists.txt: "# ITKSmoothingRecursiveGaussianImage"), so although its old UUID is still in
// the plugin replacement map, createFilter() returns null and there is NO live-ITK (B) parity arm to run. We assert
// that unavailability below so this rationale is verified in-code (and this test starts running a (B) arm if the
// legacy filter is ever re-enabled). The (rgb_image) RGB case (VM1111Shrink-RGB.png) is SKIPPED (RGB, out of scope).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // NOTE: bind every side-effecting Result to a named local before SIMPLNX_RESULT_REQUIRE_VALID (that macro double-
  // evaluates its argument; inlining a ds-mutating read/execute would run it twice and the second would fail).
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SmoothingRecursiveGaussianImageFilter filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: compare to ITK's committed baseline at the ITK test's tolerance 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_SmoothingRecursiveGaussianImageFilter_default.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) unavailable: the legacy ITK filter is disabled, so it cannot be created even though its UUID is in the map.
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid);
  if(legacyUuid.has_value())
  {
    REQUIRE(Application::Instance()->getFilterList()->createFilter(*legacyUuid) == nullptr);
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}
