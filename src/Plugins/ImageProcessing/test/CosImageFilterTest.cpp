#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CosImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::CosImageFilter: Legacy parity", "[ImageProcessing][CosImageFilter]")
{
  ip_test::RunLegacyParity<CosImageFilter, float32>(*Uuid::FromString("6fe37f77-ceae-4839-9cf6-3ca7a70e14d0"), [](Arguments&) {}, 12, -3.0, 0.005);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK. (cos is defined on all reals, no domain edge.)
TEST_CASE("ImageProcessing::CosImageFilter: Legacy parity (float64)", "[ImageProcessing][CosImageFilter]")
{
  ip_test::RunLegacyParity<CosImageFilter, float64>(*Uuid::FromString("6fe37f77-ceae-4839-9cf6-3ca7a70e14d0"), [](Arguments&) {}, 12, -3.0, 0.005);
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image baseline golden cases below. Mirrors ITKCosImageTest.cpp:
// read the ITK case's input (float32 or int16) through OUR ITK-free reader, run OUR CosImageFilter (SameAsInput,
// no params), then (A) baseline compare @0.01 via ip_golden::CompareImages and (B) live-ITK parity via
// ip_golden::CompareImages at @p parityTol (0 for the integer case -> bit-exact; 1e-4 for the float case).
// Pinned ForceInCore: the legacy ITK filter bad_casts an OOC store (reading via OUR reader is OOC-safe).
// -----------------------------------------------------------------------------
void RunCosItkGolden(const std::string& inputFile, const std::string& baselineFile, float64 parityTol)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(CosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(CosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(CosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  CosImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds.
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<CosImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(CosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, parityTol);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::CosImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][CosImageFilter]")
{
  RunCosItkGolden("RA-Slice-Float.nrrd", "BasicFilters_CosImageFilter_float.nrrd", /*parityTol=*/1.0e-4);
}

TEST_CASE("ImageProcessing::CosImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][CosImageFilter]")
{
  RunCosItkGolden("RA-Slice-Short.nrrd", "BasicFilters_CosImageFilter_short.nrrd", /*parityTol=*/0.0);
}
