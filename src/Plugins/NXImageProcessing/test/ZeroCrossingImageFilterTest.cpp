#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using ZCFilter = ZeroCrossingImageFilter;

// Sets the geometry, input, and output keys plus the foreground and background values on the Arguments. The helper
// runs preflight and execute, and requires that both steps succeed.
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
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: an unsigned input is rejected (SignedScalar); a multi-component input is rejected
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
// ITK-sourced real-image golden -- duplicates ITKZeroCrossingImageTest.cpp on our ITK-free filter (2th_cthead1_distance
// .nrrd input). The output is a fixed uint8 image (AlwaysUInt8). The durable golden is an md5 pin. The helper pins
// ForceInCore.
//   defaults: the filter defaults (ForegroundValue 1, BackgroundValue 0). They agree with the ITK defaults.
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
