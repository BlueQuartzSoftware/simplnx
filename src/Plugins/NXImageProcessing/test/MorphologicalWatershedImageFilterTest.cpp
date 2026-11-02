#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MorphologicalWatershedImageFilter.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

// ITK-free composite: HMinima -> RegionalMinima -> ConnectedComponent -> FAH flood.

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using WSFilter = MorphologicalWatershedImageFilter;

// Sets the standard geom/input/output keys + Level/MarkWatershedLine/FullyConnected on a shared Arguments, runs
// preflight + execute, and requires both succeed.
void RunWatershed(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 level, bool markWatershedLine, bool fullyConnected, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(WSFilter::k_Level_Key, std::make_any<float64>(level));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

} // namespace

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint32 output array (AlwaysUInt32).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: preflight guards", "[ImageProcessing][MorphologicalWatershedImageFilter]")
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
    WSFilter filter;
    Arguments args;
    args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(WSFilter::k_Level_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a uint32 output")
  {
    DataStructure ds;
    const std::vector<float32> field = rt::MakePlateauPattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    WSFilter filter;
    RunWatershed(filter, ds, inputPath, /*level=*/0.0, /*markWatershedLine=*/true, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint32);
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight RAM-fit guard: ValidateWatershedFitsInMemory hard-errors (with the code this filter passes, -79042)
//     for an absurd tuple count whose working set exceeds ANY machine's RAM, WITHOUT allocating an array; a small
//     tuple count is accepted on the test machine. No specific available-RAM number is asserted -- only that an absurd
//     size is rejected and a plausibly-small size is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: preflight RAM-fit guard", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  // ~1e11 voxels * >=11 bytes/voxel ~= 1.1 TB working set: larger than any machine's RAM, so it must be rejected with
  // this filter's memory code (-79042). The guard is a pure numeric estimate -- nothing is allocated. The ERROR takes
  // precedence over the OOC warning, so inputIsOutOfCore is irrelevant here.
  const Result<OutputActions> tooLarge = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(100'000'000'000ULL), /*inputIsOutOfCore=*/true, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_INVALID(tooLarge);
  REQUIRE(tooLarge.errors().front().code == -79042);

  // A tiny volume trivially fits on any test machine. In-core input: accepted with NO warning (RAM is the expected
  // backing -- no surprise to report).
  const Result<OutputActions> tinyInCore = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/false, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyInCore);
  REQUIRE(tinyInCore.warnings().empty());

  // Same tiny volume, but the input is stored out-of-core: still VALID (it fits), yet now carries the OOC-override
  // warning with this filter's warning code (-79044) -- watershed will load its working set into RAM regardless.
  const Result<OutputActions> tinyOoc = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/true, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyOoc);
  REQUIRE(tinyOoc.warnings().size() == 1);
  REQUIRE(tinyOoc.warnings().front().code == -79044);
}

// -----------------------------------------------------------------------------
// (4) FromSIMPLJson: the legacy SIMPL parameter set (Level + the two bools + geom/array/name) round-trips onto the
//     new filter's Arguments.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: FromSIMPLJson", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  const nlohmann::json json = {{"Level", 4.5},
                               {"MarkWatershedLine", false},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Watershed"}};

  Result<Arguments> result = MorphologicalWatershedImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(MorphologicalWatershedImageFilter::k_Level_Key) == 4.5);
  REQUIRE(args.value<bool>(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key) == false);
  REQUIRE(args.value<bool>(MorphologicalWatershedImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(MorphologicalWatershedImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(MorphologicalWatershedImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(MorphologicalWatershedImageFilter::k_OutputImageArrayName_Key) == "Watershed");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKMorphologicalWatershedImageTest.cpp on OUR ITK-free filter
// (cthead1-grad-mag.nrrd input). Output is a FIXED uint32 label image (AlwaysUInt32); the DURABLE golden is
// md5-validity-first (plan Sec.4). The helper pins ForceInCore.
//   defaults: filter defaults (Level 0.0, MarkWatershedLine true, FullyConnected false -- identical vs legacy ITK).
//   level_1:  Level = 1.0, MarkWatershedLine = false.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MorphologicalWatershedImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<MorphologicalWatershedImageFilter>("cthead1-grad-mag.nrrd", "406079d7904d4e9ab0b5f29f7a3a1ea8");
}

TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: ITK real-image golden (level_1)", "[ImageProcessing][ItkGolden][MorphologicalWatershedImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<MorphologicalWatershedImageFilter>("cthead1-grad-mag.nrrd", "a204ce7cf8ec4e7bc6538f0515a8910e", [](Arguments& args) {
    args.insertOrAssign(MorphologicalWatershedImageFilter::k_Level_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(false));
  });
}
