#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/BinaryThinningImageFilter.hpp"

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
using BtFilterType = BinaryThinningImageFilter;

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
void RunThinning(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(BtFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(BtFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BtFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
//     rejected (IntegerOnly); a valid integer scalar input produces a SameAsInput output array.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryThinningImageFilter: preflight guards", "[NXImageProcessing][BinaryThinningImageFilter]")
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
    BtFilterType filter;
    Arguments args;
    args.insertOrAssign(BtFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BtFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(BtFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float input rejected")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 6, 6, 1, std::vector<float32>(36, 1.0f));
    const DataPath geomPath = inputPath.getParent().getParent();
    BtFilterType filter;
    Arguments args;
    args.insertOrAssign(BtFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(BtFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BtFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    const auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.output_actions.invalid()); // float32 is not an IntegerOnly type
  }
  SECTION("scalar integer input creates a SameAsInput output")
  {
    DataStructure dataStructure;
    const std::vector<uint8> field = MakeSolidRectField<uint8>(15, 15);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 15, 15, 1, field);
    BtFilterType filter;
    RunThinning(filter, dataStructure, inputPath);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (2) FromSIMPLJson: the geometry/array/name DataPaths (thinning has no algorithm params). The geometry and the
//     input array BOTH read the same SIMPL key "SelectedCellArrayPath"; the output name reads "NewCellArrayName".
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryThinningImageFilter: FromSIMPLJson", "[NXImageProcessing][BinaryThinningImageFilter]")
{
  const nlohmann::json json = {
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "ThinnedOut"},
  };
  const Result<Arguments> result = BinaryThinningImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<DataPath>(BtFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(BtFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(BtFilterType::k_OutputImageArrayName_Key) == "ThinnedOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKBinaryThinningImageTest.cpp on our ITK-free filter (BlackDots.png ->
// uint8, SameAsInput). The filter has no algorithm parameters. The durable golden is an md5 pin. The helper pins
// ForceInCore. The ITK test's second case is "SIMPL Backwards Compatibility". The FromSIMPLJson test covers that case.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryThinningImageFilter: ITK real-image golden (BinaryThinning)", "[NXImageProcessing][ItkGolden][BinaryThinningImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<BinaryThinningImageFilter>("BlackDots.png", "153ad0b2f3658dee3b14ad93d0cfe550");
}
