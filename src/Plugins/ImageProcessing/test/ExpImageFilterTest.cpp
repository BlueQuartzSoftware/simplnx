#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ExpImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::ExpImageFilter: Legacy parity", "[ImageProcessing][ExpImageFilter]")
{
  ip_test::RunLegacyParity<ExpImageFilter, float32>(*Uuid::FromString("264977e7-cc0d-4d2b-ba9c-a30765b498b2"), [](Arguments&) {}, 12, -5.0, 0.01);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::ExpImageFilter: Legacy parity (float64)", "[ImageProcessing][ExpImageFilter]")
{
  ip_test::RunLegacyParity<ExpImageFilter, float64>(*Uuid::FromString("264977e7-cc0d-4d2b-ba9c-a30765b498b2"), [](Arguments&) {}, 12, -5.0, 0.01);
}

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::exp(static_cast<double>(x))).
// exp overflows double above ~709.78, so exp(1000) is +inf and casting +inf to float32 stays +inf. ITK's Exp
// functor is the same expression, so this pins the observed +inf overflow.
TEST_CASE("ImageProcessing::ExpImageFilter: exp of a large input overflows to +inf (domain edge)", "[ImageProcessing][ExpImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 and (B) live-ITK parity (1e-4).
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ExpImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ExpImageFilter]")
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
  args.insertOrAssign(ExpImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ExpImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  ExpImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_ExpImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<ExpImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
