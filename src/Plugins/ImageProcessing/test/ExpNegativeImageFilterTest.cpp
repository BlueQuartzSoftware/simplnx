#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ExpNegativeImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::ExpNegativeImageFilter: Legacy parity", "[ImageProcessing][ExpNegativeImageFilter]")
{
  ip_test::RunLegacyParity<ExpNegativeImageFilter, float32>(*Uuid::FromString("2c84cc7c-01ab-4550-9ba5-b9fa58b74599"), [](Arguments&) {}, 12, -5.0, 0.01);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::ExpNegativeImageFilter: Legacy parity (float64)", "[ImageProcessing][ExpNegativeImageFilter]")
{
  ip_test::RunLegacyParity<ExpNegativeImageFilter, float64>(*Uuid::FromString("2c84cc7c-01ab-4550-9ba5-b9fa58b74599"), [](Arguments&) {}, 12, -5.0, 0.01);
}

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::exp(-static_cast<double>(x))).
// For a large NEGATIVE input, -x is large positive, so exp(-x) overflows double (>~709.78) to +inf, and casting
// +inf to float32 stays +inf. ITK's ExpNegative functor is the same expression, so this pins the +inf overflow.
TEST_CASE("ImageProcessing::ExpNegativeImageFilter: exp(-x) of a large negative input overflows to +inf (domain edge)", "[ImageProcessing][ExpNegativeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpNegativeImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // exp(-(-1000)) = exp(1000) = +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpNegativeImageTest.cpp(defaults):
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 and
// (B) live-ITK parity (1e-4). See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ExpNegativeImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ExpNegativeImageFilter]")
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
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  ExpNegativeImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_ExpNegativeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<ExpNegativeImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
