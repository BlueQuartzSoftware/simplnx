#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/OpeningByReconstructionImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
rt::ReconParamSetter BallSetter(bool preserveIntensities)
{
  return rt::SEReconParamSetter<OpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, preserveIntensities);
}
} // namespace

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: opening by reconstruction removes a bright structure smaller
//     than the structuring element (an isolated 1-voxel dot) but restores a bright structure larger than it (a
//     5x5 square) to its original value. A plain opening would round the square's corners; reconstruction
//     regrows it exactly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OpeningByReconstructionImageFilter: hand-computed remove-dot vs restore-square", "[ImageProcessing][OpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 11;
  constexpr usize DY = 11;
  constexpr usize DZ = 1;
  constexpr int32 background = 0;
  constexpr int32 bright = 100;

  std::vector<int32> pattern(DX * DY * DZ, background);
  // A 5x5 bright square (interior) -- larger than a radius-1 Ball, so its interior survives erosion and is regrown.
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = bright;
    }
  }
  // An isolated 1-voxel bright dot -- smaller than the structuring element, so erosion removes it with no seed to
  // reconstruct from.
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = bright;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  OpeningByReconstructionImageFilter filter;
  rt::RunReconstructionFilter<OpeningByReconstructionImageFilter>(filter, ds, inputPath, BallSetter(/*preserveIntensities=*/false), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == background); // isolated dot removed
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == bright);     // square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == bright);     // square corner regrown (a plain opening would round it off)
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == background); // untouched background
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed remove-dot/restore-square image. Opening by
//      reconstruction routes its erode + geodesic reconstruction through ApplyMorphology's and
//      ApplyMorphologicalReconstruction's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the
//      in-core and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The
//      hand-derived expectations (isolated dot removed, square restored exactly) are asserted directly -- an
//      independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OpeningByReconstructionImageFilter: hand-computed remove-dot vs restore-square (both algorithm paths)", "[ImageProcessing][OpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 11, DY = 11, DZ = 1;
  constexpr int32 background = 0;
  constexpr int32 bright = 100;
  std::vector<int32> pattern(DX * DY * DZ, background);
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = bright; // 5x5 bright square -- larger than the SE, regrown exactly
    }
  }
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = bright; // isolated 1-voxel dot -- smaller than the SE, removed

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  OpeningByReconstructionImageFilter filter;
  Arguments args;
  args.insertOrAssign(OpeningByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(OpeningByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OpeningByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  BallSetter(/*preserveIntensities=*/false)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == background); // isolated dot removed
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == bright);     // square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == bright);     // square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == background); // untouched background
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::OpeningByReconstructionImageFilter: FromSIMPLJson", "[ImageProcessing][OpeningByReconstructionImageFilter]")
{
  const nlohmann::json json = {{"KernelType", 2},
                               {"KernelRadius", {{"x", 2}, {"y", 3}, {"z", 4}}},
                               {"FullyConnected", true},
                               {"PreserveIntensities", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Opened"}};

  Result<Arguments> result = OpeningByReconstructionImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<uint64>(OpeningByReconstructionImageFilter::k_KernelType_Key) == 2);
  REQUIRE(args.value<std::vector<uint32>>(OpeningByReconstructionImageFilter::k_KernelRadius_Key) == std::vector<uint32>{2, 3, 4});
  REQUIRE(args.value<bool>(OpeningByReconstructionImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<bool>(OpeningByReconstructionImageFilter::k_PreserveIntensities_Key) == true);
  REQUIRE(args.value<DataPath>(OpeningByReconstructionImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(OpeningByReconstructionImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(OpeningByReconstructionImageFilter::k_OutputImageArrayName_Key) == "Opened");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKOpeningByReconstructionImageTest.cpp(OpeningByReconstruction):
// STAPLE1.png (uint8), Ball radius {1,1,1} (FullyConnected + PreserveIntensities at ITK defaults). (B) live-ITK
// bit-exact + (A) md5-validity-first. The committed hash 095f00a6... is the identity-output hash (this
// opening-by-reconstruction leaves STAPLE1 unchanged), so it reproduces exactly if the reader is byte-correct.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OpeningByReconstructionImageFilter: ITK real-image golden (OpeningByReconstruction)", "[ImageProcessing][ItkGolden][OpeningByReconstructionImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<OpeningByReconstructionImageFilter>("STAPLE1.png", "095f00a68a84df4396914fa758f34dcc", [](Arguments& args) {
    args.insertOrAssign(OpeningByReconstructionImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball)));
    args.insertOrAssign(OpeningByReconstructionImageFilter::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(std::vector<uint32>{1, 1, 1}));
  });
}
