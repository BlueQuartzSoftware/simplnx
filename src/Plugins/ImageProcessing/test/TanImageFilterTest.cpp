#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/TanImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): tan has a pole at pi/2. Because pi/2 is irrational the
// nearest float32 to it is NOT exactly the pole, so std::tan(static_cast<double>(x)) is a very large FINITE value
// (never inf) -- and ITK's Tan functor is the same expression, so this pins that observed large-but-finite output
// (asserted both by magnitude and by exact equality to the recomputed op).
TEST_CASE("ImageProcessing::TanImageFilter: tan near pi/2 is large-magnitude finite (domain edge)", "[ImageProcessing][TanImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr float64 k_HalfPi = 1.5707963267948966; // double nearest pi/2
  const float32 inValue = static_cast<float32>(k_HalfPi);
  const float32 expected = static_cast<float32>(std::tan(static_cast<double>(inValue)));

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {inValue});
  const DataPath output = ip_test::RunPointwiseFilter<TanImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  const float32 value = out.getValue(0);
  REQUIRE(std::isfinite(value));     // NOT inf: float32 cannot represent the exact pole
  REQUIRE(std::abs(value) > 1.0e6f); // but is a large magnitude
  REQUIRE(value == expected);        // exact: identical std::tan-in-double formula as the filter op
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKTanImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::TanImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][TanImageFilter]")
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
  args.insertOrAssign(TanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(TanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(TanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  TanImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_TanImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
