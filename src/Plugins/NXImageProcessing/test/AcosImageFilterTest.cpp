#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AcosImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior; does NOT "fix" it): acos is undefined for |x|>1. The op is
// static_cast<U>(std::acos(static_cast<double>(x))) -- exactly ITK's equally-unguarded Acos functor -- so acos(2)
// is IEEE NaN. This pins that observed output.
TEST_CASE("ImageProcessing::AcosImageFilter: acos of |x|>1 is NaN (domain edge)", "[ImageProcessing][AcosImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAcosImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR ITK-free reader, run OUR AcosImageFilter with the ITK case's
// params (defaults), then compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at
// the ITK test's tolerance 0.01 via ip_golden::CompareImages. CompareImages dispatches on the runtime type and --
// unlike CompareDataArrays -- tolerates the non-finite tails these functors can emit.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AcosImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AcosImageFilter]")
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
  args.insertOrAssign(AcosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AcosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AcosImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01 (SameAsInput output owned by 'geom').
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AcosImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
