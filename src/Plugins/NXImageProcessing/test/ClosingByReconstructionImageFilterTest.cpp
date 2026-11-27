#include "MorphologyFilterTestUtils.hpp"

#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ClosingByReconstructionImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
rt::ReconParamSetterType BallSetter(bool preserveIntensities)
{
  return rt::SEReconParamSetter<ClosingByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, preserveIntensities);
}
} // namespace

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image (dual of opening by reconstruction): closing by reconstruction
//     removes a DARK structure smaller than the structuring element (an isolated 1-voxel dark dot on a bright
//     field) but restores a dark structure larger than it (a 5x5 dark square) to its original value/contours.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: hand-computed remove-dot vs restore-square", "[NXImageProcessing][ClosingByReconstructionImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 11;
  constexpr usize k_Dy = 11;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 100; // bright background
  constexpr int32 k_Dark = 0;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  // A 5x5 dark square (interior) -- larger than a radius-1 Ball, so its interior survives dilation and is regrown.
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, k_Dx, k_Dy)] = k_Dark;
    }
  }
  // An isolated 1-voxel dark dot -- smaller than the structuring element, so dilation fills it with no seed to
  // reconstruct from.
  pattern[rt::FlatIndex(9, 1, 0, k_Dx, k_Dy)] = k_Dark;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  ClosingByReconstructionImageFilter filter;
  rt::RunReconstructionFilter<ClosingByReconstructionImageFilter>(filter, dataStructure, inputPath, BallSetter(/*preserveIntensities=*/false), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, k_Dx, k_Dy)) == k_Field); // isolated dark dot removed (filled to field)
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, k_Dx, k_Dy)) == k_Dark);  // dark square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Dark);  // dark square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, k_Dx, k_Dy)) == k_Field); // untouched bright field
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed remove-dot/restore-square image (dual of opening).
//      Closing by reconstruction routes its dilate + geodesic reconstruction-by-erosion through ApplyMorphology's
//      and ApplyMorphologicalReconstruction's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH
//      the in-core and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH).
//      The hand-derived expectations (isolated dark dot filled, dark square restored) are asserted directly -- an
//      independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: hand-computed remove-dot vs restore-square (both algorithm paths)", "[NXImageProcessing][ClosingByReconstructionImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_Dx = 11;
  constexpr usize k_Dy = 11;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 100; // bright background
  constexpr int32 k_Dark = 0;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, k_Dx, k_Dy)] = k_Dark; // 5x5 dark square -- larger than the SE, regrown exactly
    }
  }
  pattern[rt::FlatIndex(9, 1, 0, k_Dx, k_Dy)] = k_Dark; // isolated 1-voxel dark dot -- smaller than the SE, filled

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  ClosingByReconstructionImageFilter filter;
  Arguments args;
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  BallSetter(/*preserveIntensities=*/false)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, k_Dx, k_Dy)) == k_Field); // isolated dark dot filled to field
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, k_Dx, k_Dy)) == k_Dark);  // dark square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Dark);  // dark square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, k_Dx, k_Dy)) == k_Field); // untouched bright field
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: FromSIMPLJson", "[NXImageProcessing][ClosingByReconstructionImageFilter]")
{
  const nlohmann::json json = {{"KernelType", 2},
                               {"KernelRadius", {{"x", 2}, {"y", 3}, {"z", 4}}},
                               {"FullyConnected", true},
                               {"PreserveIntensities", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Closed"}};

  Result<Arguments> result = ClosingByReconstructionImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<uint64>(ClosingByReconstructionImageFilter::k_KernelType_Key) == 2);
  REQUIRE(args.value<std::vector<uint32>>(ClosingByReconstructionImageFilter::k_KernelRadius_Key) == std::vector<uint32>{2, 3, 4});
  REQUIRE(args.value<bool>(ClosingByReconstructionImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<bool>(ClosingByReconstructionImageFilter::k_PreserveIntensities_Key) == true);
  REQUIRE(args.value<DataPath>(ClosingByReconstructionImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(ClosingByReconstructionImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(ClosingByReconstructionImageFilter::k_OutputImageArrayName_Key) == "Closed");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKClosingByReconstructionImageTest.cpp(ClosingByReconstruction):
// STAPLE1.png (uint8), Ball radius {1,1,1}. (B) live-ITK bit-exact + (A) md5-validity-first. The committed hash
// 095f00a6... is the identity-output hash (this closing-by-reconstruction leaves STAPLE1 unchanged).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: ITK real-image golden (ClosingByReconstruction)", "[NXImageProcessing][ItkGolden][ClosingByReconstructionImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ClosingByReconstructionImageFilter>("STAPLE1.png", "095f00a68a84df4396914fa758f34dcc", [](Arguments& args) {
    args.insertOrAssign(ClosingByReconstructionImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball)));
    args.insertOrAssign(ClosingByReconstructionImageFilter::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(std::vector<uint32>{1, 1, 1}));
  });
}

TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: Kernel Radius representability", "[NXImageProcessing][ClosingByReconstructionImageFilter]")
{
  morph_test::RequireKernelRadiusPreflight<ClosingByReconstructionImageFilter>();
}

TEST_CASE("NXImageProcessing::ClosingByReconstructionImageFilter: Ordinary Kernel Radius control", "[NXImageProcessing][ClosingByReconstructionImageFilter]")
{
  morph_test::RequireOrdinaryKernelRadius<ClosingByReconstructionImageFilter>({5, 5, 5});
}
