#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AsinImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::AsinImageFilter: Legacy parity", "[ImageProcessing][AsinImageFilter]")
{
  ip_test::RunLegacyParity<AsinImageFilter, float32>(*Uuid::FromString("1b463492-041f-4680-abb1-0b94a3019063"), [](Arguments&) {}, 12, -1.0, 2.0 / 1727.0);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 in-domain ramp ([-1,1]) so the
// otherwise-uninstantiated float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::AsinImageFilter: Legacy parity (float64)", "[ImageProcessing][AsinImageFilter]")
{
  ip_test::RunLegacyParity<AsinImageFilter, float64>(*Uuid::FromString("1b463492-041f-4680-abb1-0b94a3019063"), [](Arguments&) {}, 12, -1.0, 2.0 / 1727.0);
}

// PART A domain edge (documents ITK-faithful behavior): asin is undefined for |x|>1. The op is
// static_cast<U>(std::asin(static_cast<double>(x))) -- ITK's equally-unguarded Asin functor -- so asin(2) is NaN.
TEST_CASE("ImageProcessing::AsinImageFilter: asin of |x|>1 is NaN (domain edge)", "[ImageProcessing][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AsinImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAsinImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 via ip_golden::CompareImages and
// (B) live-ITK parity at the float-math class (1e-4). See AcosImageFilterTest.cpp for the full rationale (CompareImages
// dispatches on runtime type and tolerates non-finite tails; whole case is ForceInCore for the legacy leg).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AsinImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AsinImageFilter]")
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
  args.insertOrAssign(AsinImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AsinImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AsinImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AsinImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<AsinImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
