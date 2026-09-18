#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"
using namespace nx::core;

TEST_CASE("ImageProcessing::NormalizeImageFilter: Legacy parity", "[ImageProcessing][NormalizeImageFilter]")
{
  // Use a non-degenerate ramp (nonzero variance). Output is float64; parity via CompareDataArrays tolerance.
  ip_test::RunLegacyParity<NormalizeImageFilter>(*Uuid::FromString("9d8ce30e-c75e-4ca8-b6be-0b11baa7e6ce"), [](Arguments&) {}, 12, -100.0, 0.25);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKNormalizeImageTest.cpp(defaults): read
// Ramp-Up-Short.nrrd (int16) through OUR reader, run OUR NormalizeImageFilter (AlwaysFloat64 output, no params), then:
//   (A) DURABLE golden -- compare our float64 output to ITK's committed baseline .nrrd (type: double, SAME reader) at
//       the ITK test's tolerance 0.0001 via ip_golden::CompareImages.
//   (B) LIVE-ITK parity -- legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
// The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an RGB/vector input (plan Sec.5; it is also disabled in
// the ITK source). Pinned ForceInCore: the legacy ITK filter bad_casts an OOC store.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::NormalizeImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][NormalizeImageFilter]")
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
  args.insertOrAssign(NormalizeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NormalizeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NormalizeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NormalizeImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_NormalizeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<NormalizeImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(NormalizeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
