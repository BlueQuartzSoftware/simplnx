#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"

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
using TMCCFilter = ThresholdMaximumConnectedComponentsImageFilter;

// Sets the geometry, input, and output keys plus the four operation parameters on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunTMCC(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue,
             const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(minimumObjectSizeInPixels));
  args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(upperBoundary));
  args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(insideValue));
  args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(outsideValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: preflight guards", "[ImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]")
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
    TMCCFilter filter;
    Arguments args;
    args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(0u));
    args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
    args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input (incl. float) creates a uint8 output")
  {
    DataStructure ds;
    const std::vector<float32> field = rt::MakeGradientNoisePattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    TMCCFilter filter;
    RunTMCC(filter, ds, inputPath, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/65536.0, /*insideValue=*/1u, /*outsideValue=*/0u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKThresholdMaximumConnectedComponentsImageTest.cpp on our ITK-free
// filter. The output is a fixed uint8 binary image (AlwaysUInt8). The durable golden is an md5 pin. The helper pins
// ForceInCore.
//   default:    cthead1.png, defaults (MinimumObjectSizeInPixels 0, UpperBoundary 65536, Inside 1, Outside 0).
//   parameters: cthead1.png, MinimumObjectSizeInPixels = 40 AND UpperBoundary = 150.
//   float:      RA-Float.nrrd (float32 input, uint8 output), defaults.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "c84b75c78c33844251a1095d9cbcffb9");
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (parameters)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "27c6cf8494fcc4e414f1c420e7a9ca6f", [](Arguments& args) {
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(40u));
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(150.0));
  });
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("RA-Float.nrrd", "e475b27bd0dd66ede330c4eab93c17e9");
}
