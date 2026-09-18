#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AtanImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::AtanImageFilter: Legacy parity", "[ImageProcessing][AtanImageFilter]")
{
  ip_test::RunLegacyParity<AtanImageFilter, float32>(*Uuid::FromString("39933f50-088c-46ac-a421-d238f1b178fd"), [](Arguments&) {}, 12, -100.0, 0.1);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK. (atan is defined on all reals, no domain edge.)
TEST_CASE("ImageProcessing::AtanImageFilter: Legacy parity (float64)", "[ImageProcessing][AtanImageFilter]")
{
  ip_test::RunLegacyParity<AtanImageFilter, float64>(*Uuid::FromString("39933f50-088c-46ac-a421-d238f1b178fd"), [](Arguments&) {}, 12, -100.0, 0.1);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAtanImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 and (B) live-ITK parity (1e-4).
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AtanImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AtanImageFilter]")
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
  args.insertOrAssign(AtanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AtanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AtanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AtanImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AtanImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<AtanImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(AtanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
