#include "NXImageProcessing/Filters/NormalizeToConstantImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"
using namespace nx::core;

TEST_CASE("NXImageProcessing::NormalizeToConstantImageFilter: Correctness", "[NXImageProcessing][NormalizeToConstantImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  constexpr usize k_Dim = 12;
  const double constant = 100.0;

  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(dataStructure, k_Dim, 1.0, 1.0); // values 1..1728 -> sum > 0
  NormalizeToConstantImageFilter filter;
  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(constant));
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const usize n = k_Dim * k_Dim * k_Dim;
  const double sum = static_cast<double>(n) * (1.0 + static_cast<double>(n)) / 2.0; // sum of 1..n
  const double factor = constant / sum;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outStore = dataStructure.getDataRefAs<Float64ArrayType>(outputPath).getDataStoreRef();
  for(usize i = 0; i < n; ++i)
  {
    const double x = 1.0 + static_cast<double>(i);
    REQUIRE(outStore.getValue(i) == Approx(x * factor).margin(1.0e-12));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::NormalizeToConstantImageFilter: Zero-sum error", "[NXImageProcessing][NormalizeToConstantImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(dataStructure, 8, 0.0, 0.0); // all zeros -> sum == 0
  NormalizeToConstantImageFilter filter;
  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(1.0));
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKNormalizeToConstantImageTest.cpp(defaults): read
// Ramp-Up-Short.nrrd (int16) through OUR reader, run OUR NormalizeToConstantImageFilter (AlwaysFloat64Type output, no
// params -- Constant default 1.0, shared with the legacy filter), then:
//   (A) DURABLE golden -- compare our float64 output to ITK's committed baseline .nrrd (type: double, SAME reader) at
//       the ITK test's tolerance 0.0001 via ip_golden::CompareImages.
// The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an RGB/vector input (plan Sec.5). Pinned ForceInCore for
// the reader (kept for consistency with the other golden cases; only OUR reader + filter run here).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::NormalizeToConstantImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][NormalizeToConstantImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("Ramp-Up-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NormalizeToConstantImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_NormalizeToConstantImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
