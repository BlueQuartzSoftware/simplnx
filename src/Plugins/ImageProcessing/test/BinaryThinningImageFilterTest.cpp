#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryThinningImageFilter.hpp"

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
using BTFilter = BinaryThinningImageFilter;

// A single-slice 2D field that holds one solid rectangle. The rectangle spans x=3..11 and y=3..11 with the
// foreground value 1. The background value is 0. The rectangle thins to a medial skeleton.
template <class T>
std::vector<T> MakeSolidRectField(usize dimX, usize dimY)
{
  std::vector<T> v(dimX * dimY, T{0});
  for(usize y = 3; y <= 11 && y < dimY; ++y)
  {
    for(usize x = 3; x <= 11 && x < dimX; ++x)
    {
      v[rt::FlatIndex(x, y, 0, dimX, dimY)] = T{1};
    }
  }
  return v;
}

// Sets the geometry, input, and output keys on the Arguments. The filter has no algorithm parameters. The helper
// runs preflight and execute, and requires that both steps succeed.
void RunThinning(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
//     rejected (IntegerOnly); a valid integer scalar input produces a SameAsInput output array.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: preflight guards", "[ImageProcessing][BinaryThinningImageFilter]")
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
    BTFilter filter;
    Arguments args;
    args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float input rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    const DataPath geomPath = inputPath.getParent().getParent();
    BTFilter filter;
    Arguments args;
    args.insertOrAssign(BTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
  }
  SECTION("scalar integer input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeSolidRectField<uint8>(15, 15);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 15, 15, 1, field);
    BTFilter filter;
    RunThinning(filter, ds, inputPath);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (2) FromSIMPLJson: the geometry/array/name DataPaths (thinning has no algorithm params). The geometry and the
//     input array BOTH read the same SIMPL key "SelectedCellArrayPath"; the output name reads "NewCellArrayName".
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: FromSIMPLJson", "[ImageProcessing][BinaryThinningImageFilter]")
{
  const nlohmann::json json = {
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "ThinnedOut"},
  };
  const Result<Arguments> result = BinaryThinningImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<DataPath>(BTFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(BTFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(BTFilter::k_OutputImageArrayName_Key) == "ThinnedOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKBinaryThinningImageTest.cpp on our ITK-free filter (BlackDots.png ->
// uint8, SameAsInput). The filter has no algorithm parameters. The durable golden is an md5 pin. The helper pins
// ForceInCore. The ITK test's second case is "SIMPL Backwards Compatibility". The FromSIMPLJson test covers that case.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryThinningImageFilter: ITK real-image golden (BinaryThinning)", "[ImageProcessing][ItkGolden][BinaryThinningImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<BinaryThinningImageFilter>("BlackDots.png", "153ad0b2f3658dee3b14ad93d0cfe550");
}
