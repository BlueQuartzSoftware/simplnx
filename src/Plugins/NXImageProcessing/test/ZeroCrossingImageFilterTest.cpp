#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
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
using ZcFilterType = ZeroCrossingImageFilter;

// Sets the geometry, input, and output keys plus the foreground and background values on the Arguments. The helper
// runs preflight and execute, and requires that both steps succeed.
void RunZC(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, uint8 foregroundValue, uint8 backgroundValue, unit_test::AlgorithmTestScope* scope = nullptr,
           const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(ZcFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ZcFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ZcFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(ZcFilterType::k_ForegroundValue_Key, std::make_any<uint8>(foregroundValue));
  args.insertOrAssign(ZcFilterType::k_BackgroundValue_Key, std::make_any<uint8>(backgroundValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
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
//    ; a valid signed scalar input produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ZeroCrossingImageFilter: preflight guards", "[NXImageProcessing][ZeroCrossingImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("unsigned input rejected")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<uint16>(dataStructure, 6, 6, 1, std::vector<uint16>(36, uint16{1}));
    const DataPath geomPath = inputPath.getParent().getParent();
    ZcFilterType filter;
    Arguments args;
    args.insertOrAssign(ZcFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ZcFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ZcFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ZcFilterType::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(ZcFilterType::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions); // uint16 is not a SignedScalar type (rejected by the ArraySelectionParameter's allowed-types check)
  }
  SECTION("multi-component rejected")
  {
    DataStructure dataStructure;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<int32>(dataStructure, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    ZcFilterType filter;
    Arguments args;
    args.insertOrAssign(ZcFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ZcFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(ZcFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ZcFilterType::k_ForegroundValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(ZcFilterType::k_BackgroundValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
  }
  SECTION("scalar signed input creates a uint8 output")
  {
    DataStructure dataStructure;
    const std::vector<int16> field = MakeZeroCrossingField<int16>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(dataStructure, 12, 12, 1, field);
    ZcFilterType filter;
    RunZC(filter, dataStructure, inputPath, /*fg=*/1, /*bg=*/0);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKZeroCrossingImageTest.cpp on our ITK-free filter (2th_cthead1_distance
// .nrrd input). The output is a fixed uint8 image (AlwaysUInt8Type). The durable golden is an md5 pin. The helper pins
// ForceInCore.
//   defaults: the filter defaults (ForegroundValue 1, BackgroundValue 0). They agree with the ITK defaults.
//   inverted: ForegroundValue = 0, BackgroundValue = 2.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ZeroCrossingImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ZeroCrossingImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ZeroCrossingImageFilter>("2th_cthead1_distance.nrrd", "1b5cea61ceb46ebf606eb9e63de1e75f");
}

TEST_CASE("NXImageProcessing::ZeroCrossingImageFilter: ITK real-image golden (inverted)", "[NXImageProcessing][ItkGolden][ZeroCrossingImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ZeroCrossingImageFilter>("2th_cthead1_distance.nrrd", "17cf4374c03d958e13506db9799c4789", [](Arguments& args) {
    args.insertOrAssign(ZeroCrossingImageFilter::k_ForegroundValue_Key, std::make_any<uint8>(0));
    args.insertOrAssign(ZeroCrossingImageFilter::k_BackgroundValue_Key, std::make_any<uint8>(2));
  });
}
