#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKZeroCrossingImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the axial sign-change stencil
// engine's bit-exact output-VALUE parity gate.
const Uuid k_LegacyZCUuid = *Uuid::FromString("89a14057-776a-4e35-80b6-69361e078394");

using ZCFilter = ZeroCrossingImageFilter;

// The helper uses the shared parameter keys for the current and legacy filters.
void RunZC(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint8 foregroundValue, uint8 backgroundValue, UnitTest::AlgorithmTestScope* scope = nullptr,
           const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(ZCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ZCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ZCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(ZCFilter::k_ForegroundValue_Key, std::make_any<uint8>(foregroundValue));
  args.insertOrAssign(ZCFilter::k_BackgroundValue_Key, std::make_any<uint8>(backgroundValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic SIGNED field with genuine interior zero crossings: a centered sphere/disk indicator r^2 - k
// (negative inside the sphere of radius ~min(dimX,dimY)/3, positive outside -- so the zero level set is an interior
// closed surface) plus a mild x-ramp (breaks the symmetry so the +/-direction tie-break is exercised, not just the
// symmetric case). Values stay well within int8 range for the small parity dims (<= 12) and within int16 range for
// the 200^3 OOC volume, so a single builder serves both. Adjacent voxels straddling the boundary produce exact
// |c| == |n| ties (e.g. -1 next to +1) that stress the positive-direction tie-break.
template <class T>
std::vector<T> MakeZeroCrossingField(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  const int64 cx = static_cast<int64>(dimX / 2);
  const int64 cy = static_cast<int64>(dimY / 2);
  const int64 cz = (dimZ > 1) ? static_cast<int64>(dimZ / 2) : 0;
  const int64 radius = static_cast<int64>(std::min(dimX, dimY) / 3); // in-plane radius; the boundary stays interior
  const int64 k = radius * radius;
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const int64 ddx = static_cast<int64>(x) - cx;
        const int64 ddy = static_cast<int64>(y) - cy;
        const int64 ddz = (dimZ > 1) ? (static_cast<int64>(z) - cz) : 0;
        const int64 r2 = ddx * ddx + ddy * ddy + ddz * ddz;
        const int64 value = (r2 - k) + ddx; // sphere/disk indicator + a mild x-ramp (directional asymmetry)
        v[rt::FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(value);
      }
    }
  }
  return v;
}

// A signed field with the type-MINIMUM value adjacent to positive values exercises the magnitude of the minimum.
// ITK wraps the minimum magnitude for int8 and int16. ITK uses the exact unsigned magnitude for int32 and int64.
// A 3x3(x3) block of the minimum sits in a +100 field. The boundary voxels of the block have positive axial
// neighbors, which is a genuine sign change. Values stay in range for every signed integer type (+100 < 127).
template <class T>
std::vector<T> MakeIntMinField(usize dimX, usize dimY, usize dimZ)
{
  static_assert(std::is_signed_v<T> && std::is_integral_v<T>, "MakeIntMinField requires a signed integer type.");
  std::vector<T> v(dimX * dimY * dimZ, static_cast<T>(100));
  const usize cx = dimX / 2, cy = dimY / 2, cz = dimZ / 2;
  const usize z0 = (dimZ > 1) ? cz - 1 : 0;
  const usize z1 = (dimZ > 1) ? cz + 1 : 0;
  for(usize z = z0; z <= z1; ++z)
  {
    for(usize y = cy - 1; y <= cy + 1; ++y)
    {
      for(usize x = cx - 1; x <= cx + 1; ++x)
      {
        v[rt::FlatIndex(x, y, z, dimX, dimY)] = std::numeric_limits<T>::min();
      }
    }
  }
  return v;
}

// Requires the new output to match the legacy output exactly and to contain at least one crossing.
void RequireMatchesLegacy(const IDataArray& newOut, const IDataArray& legacyOut, uint8 foregroundValue)
{
  REQUIRE(newOut.getDataType() == DataType::uint8);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  const auto& store = newOut.getIDataStoreRefAs<AbstractDataStore<uint8>>();
  usize foregroundCount = 0;
  for(usize index = 0; index < store.getSize(); ++index)
  {
    if(store.getValue(index) == foregroundValue)
    {
      ++foregroundCount;
    }
  }
  REQUIRE(foregroundCount > 0);
  UnitTest::CompareDataArrays<uint8>(newOut, legacyOut);
}

// A 3-D field runs once per registered in-memory algorithm scenario. A single-slice field has one shared route and
// runs once without an algorithm scope.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, uint8 foregroundValue, uint8 backgroundValue)
{
  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyZCUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunZC(*legacyFilter, legacyDs, legacyInput, foregroundValue, backgroundValue);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  if(dz == 1)
  {
    DataStructure newDs;
    const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
    ZCFilter newFilter;
    RunZC(newFilter, newDs, newInput, foregroundValue, backgroundValue);
    RequireMatchesLegacy(newDs.getDataRefAs<IDataArray>(outputPath), legacyOut, foregroundValue);
    return;
  }

  for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
  {
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure newDs;
    const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
    scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
    ZCFilter newFilter;
    RunZC(newFilter, newDs, newInput, foregroundValue, backgroundValue, &scope);

    const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
    scope.requireExpectedStore(newOut);
    RequireMatchesLegacy(newOut, legacyOut, foregroundValue);
  }
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL signed type grid x {(fg,bg) settings} x {3D, 2D}. Zero-crossing detection is a
//     fixed uint8 output; the new filter must reproduce legacy ITK's marker VALUES EXACTLY (no tolerance -- see the
//     Parity model). This is the primary gate on the axial sign-change stencil engine (sign-change test, |.| ties,
//     positive-direction tie-break, ZeroFluxNeumann boundary).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ZeroCrossingImageFilter: Live-ITK parity", "[ImageProcessing][ZeroCrossingImageFilter]", int8, int16, int32, int64, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyZCUuid) != nullptr);

  // Two (foreground, background) settings: the default {1,0} and a both-nonzero {255,7}.
  const uint8 foregroundValue = GENERATE(uint8{1}, uint8{255});
  const uint8 backgroundValue = (foregroundValue == uint8{1}) ? uint8{0} : uint8{7};
  CAPTURE(foregroundValue, backgroundValue);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(MakeZeroCrossingField<T>(DX, DY, DZ), DX, DY, DZ, foregroundValue, backgroundValue);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(MakeZeroCrossingField<T>(DX, DY, DZ), DX, DY, DZ, foregroundValue, backgroundValue);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: an unsigned input is rejected (SignedScalar); a multi-component input is rejected
//     (requireScalar); a valid signed scalar input produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ZeroCrossingImageFilter: preflight guards", "[ImageProcessing][ZeroCrossingImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("unsigned input rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<uint16>(ds, 6, 6, 1, std::vector<uint16>(36, uint16{1}));
    const DataPath geomPath = inputPath.getParent().getParent();
    ZCFilter filter;
    Arguments args;
    args.insertOrAssign(ZCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ZCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ZCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ZCFilter::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(ZCFilter::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions); // uint16 is not a SignedScalar type (rejected by the ArraySelectionParameter's allowed-types check)
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    ZCFilter filter;
    Arguments args;
    args.insertOrAssign(ZCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ZCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(ZCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ZCFilter::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(ZCFilter::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar signed input creates a uint8 output")
  {
    DataStructure ds;
    const std::vector<int16> field = MakeZeroCrossingField<int16>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 12, 12, 1, field);
    ZCFilter filter;
    RunZC(filter, ds, inputPath, /*fg=*/1, /*bg=*/0);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (4) INT_MIN corner uses a type-minimum value next to a positive value.
//     The magnitude model wraps int8 and int16 minima to negative pixel values.
//     The model compares int32 and int64 minima as exact unsigned magnitudes.
//     The result must match live ITK for every signed integer width.
//     This test uses integer types because floating-point types do not have this corner case.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ZeroCrossingImageFilter: INT_MIN adjacent to positive matches live ITK", "[ImageProcessing][ZeroCrossingImageFilter]", int8, int16, int32, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyZCUuid) != nullptr);

  SECTION("3D")
  {
    constexpr usize DX = 6, DY = 6, DZ = 4;
    RequireParity<T>(MakeIntMinField<T>(DX, DY, DZ), DX, DY, DZ, /*fg=*/1, /*bg=*/0);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 6, DY = 6, DZ = 1;
    RequireParity<T>(MakeIntMinField<T>(DX, DY, DZ), DX, DY, DZ, /*fg=*/1, /*bg=*/0);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKZeroCrossingImageTest.cpp on OUR ITK-free filter (2th_cthead1_distance
// .nrrd input). Output is a FIXED uint8 image (AlwaysUInt8); (A) DURABLE golden = md5-validity-first (plan Sec.4),
// (B) LIVE-ITK parity = BIT-EXACT (uint8, CompareImages@0.0). Helper pins ForceInCore. No prior ItkGolden coverage
// existed in this file.
//   defaults: filter defaults (ForegroundValue 1, BackgroundValue 0 -- identical vs legacy ITK).
//   inverted: ForegroundValue = 0, BackgroundValue = 2.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ZeroCrossingImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ZeroCrossingImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ZeroCrossingImageFilter>("2th_cthead1_distance.nrrd", "1b5cea61ceb46ebf606eb9e63de1e75f");
}

TEST_CASE("ImageProcessing::ZeroCrossingImageFilter: ITK real-image golden (inverted)", "[ImageProcessing][ItkGolden][ZeroCrossingImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ZeroCrossingImageFilter>("2th_cthead1_distance.nrrd", "17cf4374c03d958e13506db9799c4789", [](Arguments& args) {
    args.insertOrAssign(ZeroCrossingImageFilter::k_ForegroundValue_Key, std::make_any<uint8>(0));
    args.insertOrAssign(ZeroCrossingImageFilter::k_BackgroundValue_Key, std::make_any<uint8>(2));
  });
}
