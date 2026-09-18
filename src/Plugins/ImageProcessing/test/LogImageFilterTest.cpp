#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LogImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::LogImageFilter: Legacy parity", "[ImageProcessing][LogImageFilter]")
{
  ip_test::RunLegacyParity<LogImageFilter, float32>(*Uuid::FromString("4b6655ad-4e6c-4e68-a771-55ca0ae40915"), [](Arguments&) {}, 12, 1.0, 1.0);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 in-domain ramp (>0) so the
// otherwise-uninstantiated float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::LogImageFilter: Legacy parity (float64)", "[ImageProcessing][LogImageFilter]")
{
  ip_test::RunLegacyParity<LogImageFilter, float64>(*Uuid::FromString("4b6655ad-4e6c-4e68-a771-55ca0ae40915"), [](Arguments&) {}, 12, 1.0, 1.0);
}

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::log(static_cast<double>(x)))
// -- ITK's equally-unguarded Log functor. log(x<0) is NaN and log(0) is -inf (IEEE pole). The in-core parity above
// starts at 1.0 so it never touches these; this pins both. (The golden test only tolerates the -inf tail, it does
// not assert it explicitly.)
TEST_CASE("ImageProcessing::LogImageFilter: log of negative is NaN and log(0) is -inf (domain edge)", "[ImageProcessing][LogImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<LogImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLogImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 and (B) live-ITK parity (1e-4).
// log(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail (identical in both engines) -- so both oracles are inf-safe here. See AcosImageFilterTest.cpp.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LogImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][LogImageFilter]")
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
  args.insertOrAssign(LogImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(LogImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  LogImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_LogImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4), inf-safe.
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<LogImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
