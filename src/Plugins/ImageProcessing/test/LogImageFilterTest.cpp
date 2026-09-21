#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LogImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::log(static_cast<double>(x)))
// -- ITK's equally-unguarded Log functor. log(x<0) is NaN and log(0) is -inf (IEEE pole). This test pins both. (The
// golden test only tolerates the -inf tail, it does not assert it explicitly.)
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
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
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

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_LogImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
