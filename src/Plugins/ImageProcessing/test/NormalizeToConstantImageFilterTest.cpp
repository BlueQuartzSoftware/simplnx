#include "ImageProcessing/Filters/NormalizeToConstantImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"
using namespace nx::core;

TEST_CASE("ImageProcessing::NormalizeToConstantImageFilter: Correctness", "[ImageProcessing][NormalizeToConstantImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  constexpr usize kDim = 12;
  const double constant = 100.0;

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, kDim, 1.0, 1.0); // values 1..1728 -> sum > 0
  NormalizeToConstantImageFilter filter;
  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(constant));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const usize n = kDim * kDim * kDim;
  const double sum = static_cast<double>(n) * (1.0 + static_cast<double>(n)) / 2.0; // sum of 1..n
  const double factor = constant / sum;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outStore = ds.getDataRefAs<Float64Array>(outputPath).getDataStoreRef();
  for(usize i = 0; i < n; ++i)
  {
    const double x = 1.0 + static_cast<double>(i);
    REQUIRE(outStore.getValue(i) == Approx(x * factor).margin(1.0e-12));
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::NormalizeToConstantImageFilter: Zero-sum error", "[ImageProcessing][NormalizeToConstantImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 8, 0.0, 0.0); // all zeros -> sum == 0
  NormalizeToConstantImageFilter filter;
  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_Constant_Key, std::make_any<float64>(1.0));
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKNormalizeToConstantImageTest.cpp(defaults): read
// Ramp-Up-Short.nrrd (int16) through OUR reader, run OUR NormalizeToConstantImageFilter (AlwaysFloat64 output, no
// params -- Constant default 1.0, shared with the legacy filter), then:
//   (A) DURABLE golden -- compare our float64 output to ITK's committed baseline .nrrd (type: double, SAME reader) at
//       the ITK test's tolerance 0.0001 via ip_golden::CompareImages.
//   (B) LIVE-ITK parity -- SKIPPED (no silent drop, plan Sec.5/Risk #8): the legacy ITKNormalizeToConstantImageFilter
//       is NOT registered in the ITKImageProcessing plugin (absent from that plugin's CMake FilterList; its own ITK
//       test instantiates the class directly rather than via the FilterList), so createFilter returns null and
//       RunLegacyItkFilter cannot run. (A) is the only available oracle here.
// The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an RGB/vector input (plan Sec.5). Pinned ForceInCore for
// the reader (kept for consistency with the other golden cases; only OUR reader + filter run here).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::NormalizeToConstantImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][NormalizeToConstantImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Up-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NormalizeToConstantImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NormalizeToConstantImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_NormalizeToConstantImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity SKIPPED: legacy ITKNormalizeToConstantImageFilter is unregistered (see note above).

  UnitTest::CheckArraysInheritTupleDims(ds);
}
