#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BoundedReciprocalImageFilter.hpp"

using namespace nx::core;

// NOTE: Unlike the other Phase-1 unary filters, there is no legacy-parity test here. The legacy
// ITKBoundedReciprocalImageFilter is disabled in the ITKImageProcessing plugin (commented out of its
// CMake FilterList because of a class-name typo defect in the legacy source), so no legacy filter is
// registered under its UUID to compare against. This new filter is therefore validated directly
// against computed-expected values: output = 1 / (1 + x), forced to float64.
TEST_CASE("ImageProcessing::BoundedReciprocalImageFilter: Correctness", "[ImageProcessing][BoundedReciprocalImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 12, 0.0, 1.0); // value(i) = i, i in [0, 1727]

  BoundedReciprocalImageFilter filter;
  Arguments args;
  args.insertOrAssign(BoundedReciprocalImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BoundedReciprocalImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BoundedReciprocalImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(outputPath)); // output is forced to float64
  auto& outStore = ds.getDataRefAs<Float64Array>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    const double x = static_cast<double>(i);
    const double expected = 1.0 / (1.0 + x);
    REQUIRE(outStore.getValue(i) == Approx(expected).margin(1.0e-9));
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKBoundedReciprocalImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR BoundedReciprocalImageFilter (AlwaysFloat64 output,
// no params), then:
//   (A) DURABLE golden -- compare our float64 output to ITK's committed baseline .nrrd (type: double, read through
//       the SAME reader) at the ITK test's tolerance 0.01 via ip_golden::CompareImages.
//   (B) LIVE-ITK parity -- SKIPPED (no silent drop, plan Sec.5/Risk #8): the legacy ITKBoundedReciprocalImageFilter
//       is DISABLED in the ITKImageProcessing plugin (class-name typo defect, see the note above), so no legacy
//       filter is registered under its UUID and RunLegacyItkFilter cannot run. (A) is the only available oracle here.
// The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an RGB/vector input (plan Sec.5).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BoundedReciprocalImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][BoundedReciprocalImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(BoundedReciprocalImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(BoundedReciprocalImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(BoundedReciprocalImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  BoundedReciprocalImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01 (our AlwaysFloat64 output matches the baseline's `type: double`).
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_BoundedReciprocalImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity SKIPPED: legacy ITKBoundedReciprocalImageFilter is disabled/unregistered (see note above).

  UnitTest::CheckArraysInheritTupleDims(ds);
}
