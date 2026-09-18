#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AcosImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::AcosImageFilter: Legacy parity", "[ImageProcessing][AcosImageFilter]")
{
  ip_test::RunLegacyParity<AcosImageFilter, float32>(*Uuid::FromString("e7411c44-95ab-4623-8bf4-59b63d2d08c5"), [](Arguments&) {}, 12, -1.0, 2.0 / 1727.0);
}

// PART B float64 dispatch coverage: the parity test above exercises only the float32 dispatch arm of AllNumeric;
// this mirrors it with a float64 in-domain ramp ([-1,1]) so the never-otherwise-instantiated float64 arm is
// covered and shown to match live ITK (which also evaluates acos in double).
TEST_CASE("ImageProcessing::AcosImageFilter: Legacy parity (float64)", "[ImageProcessing][AcosImageFilter]")
{
  ip_test::RunLegacyParity<AcosImageFilter, float64>(*Uuid::FromString("e7411c44-95ab-4623-8bf4-59b63d2d08c5"), [](Arguments&) {}, 12, -1.0, 2.0 / 1727.0);
}

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
// params (defaults), then apply the plan's two oracles:
//   (A) DURABLE golden -- compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at
//       the ITK test's tolerance 0.01 via ip_golden::CompareImages (permanent, survives ITK removal).
//   (B) LIVE-ITK parity (coexistence only) -- run the legacy ITK filter (resolved from the plugin replacement map)
//       on the SAME input into the SAME DataStructure (output "ITK Output") and compare via ip_golden::CompareImages
//       at the pointwise-float-math parity class (1e-4). CompareImages dispatches on runtime type and -- unlike
//       CompareDataArrays -- tolerates the non-finite tails these functors can emit. Pinned ForceInCore: the legacy
//       ITK filter bad_casts an OOC store (reading via OUR reader is OOC-safe).
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

  // (A) DURABLE golden: baseline compare @ 0.01 (SameAsInput output owned by 'geom').
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AcosImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<AcosImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
