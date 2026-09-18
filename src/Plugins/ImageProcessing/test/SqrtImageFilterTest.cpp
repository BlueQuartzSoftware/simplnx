#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SqrtImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::SqrtImageFilter: Legacy parity", "[ImageProcessing][SqrtImageFilter]")
{
  ip_test::RunLegacyParity<SqrtImageFilter, float32>(*Uuid::FromString("05c7c812-4e33-4e9a-bf27-d4c17f5dff68"), [](Arguments&) {}, 12, 0.0, 1.0);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 in-domain ramp (>=0) so the
// otherwise-uninstantiated float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::SqrtImageFilter: Legacy parity (float64)", "[ImageProcessing][SqrtImageFilter]")
{
  ip_test::RunLegacyParity<SqrtImageFilter, float64>(*Uuid::FromString("05c7c812-4e33-4e9a-bf27-d4c17f5dff68"), [](Arguments&) {}, 12, 0.0, 1.0);
}

// PART A domain edge (documents ITK-faithful behavior): sqrt is undefined for x<0. The op is
// static_cast<U>(std::sqrt(static_cast<double>(x))) -- ITK's equally-unguarded Sqrt functor -- so sqrt(-4) is NaN.
TEST_CASE("ImageProcessing::SqrtImageFilter: sqrt of a negative is NaN (domain edge)", "[ImageProcessing][SqrtImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-4.0f});
  const DataPath output = ip_test::RunPointwiseFilter<SqrtImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKSqrtImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then (A) baseline @0.01 and (B) live-ITK parity (1e-4).
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SqrtImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][SqrtImageFilter]")
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
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SqrtImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_SqrtImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<SqrtImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
