#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
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
using TmccFilterType = ThresholdMaximumConnectedComponentsImageFilter;

// Sets the geometry, input, and output keys plus the four operation parameters on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunTMCC(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue,
             const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(TmccFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(TmccFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TmccFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(TmccFilterType::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(minimumObjectSizeInPixels));
  args.insertOrAssign(TmccFilterType::k_UpperBoundary_Key, std::make_any<float64>(upperBoundary));
  args.insertOrAssign(TmccFilterType::k_InsideValue_Key, std::make_any<uint8>(insideValue));
  args.insertOrAssign(TmccFilterType::k_OutsideValue_Key, std::make_any<uint8>(outsideValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected; a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: preflight guards", "[NXImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure dataStructure;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<int32>(dataStructure, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    TmccFilterType filter;
    Arguments args;
    args.insertOrAssign(TmccFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(TmccFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(TmccFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(TmccFilterType::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(0u));
    args.insertOrAssign(TmccFilterType::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
    args.insertOrAssign(TmccFilterType::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(TmccFilterType::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
  }
  SECTION("scalar input (incl. float) creates a uint8 output")
  {
    DataStructure dataStructure;
    const std::vector<float32> field = rt::MakeGradientNoisePattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 12, 12, 1, field);
    TmccFilterType filter;
    RunTMCC(filter, dataStructure, inputPath, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/65536.0, /*insideValue=*/1u, /*outsideValue=*/0u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKThresholdMaximumConnectedComponentsImageTest.cpp on our ITK-free
// filter. The output is a fixed uint8 binary image (AlwaysUInt8Type). The durable golden is an md5 pin. The helper pins
// ForceInCore.
//   default:    cthead1.png, defaults (MinimumObjectSizeInPixels 0, UpperBoundary 65536, Inside 1, Outside 0).
//   parameters: cthead1.png, MinimumObjectSizeInPixels = 40 AND UpperBoundary = 150.
//   float:      RA-Float.nrrd (float32 input, uint8 output), defaults.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "c84b75c78c33844251a1095d9cbcffb9");
}

TEST_CASE("NXImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (parameters)", "[NXImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "27c6cf8494fcc4e414f1c420e7a9ca6f", [](Arguments& args) {
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(40u));
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(150.0));
  });
}

TEST_CASE("NXImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (float)", "[NXImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("RA-Float.nrrd", "e475b27bd0dd66ede330c4eab93c17e9");
}
