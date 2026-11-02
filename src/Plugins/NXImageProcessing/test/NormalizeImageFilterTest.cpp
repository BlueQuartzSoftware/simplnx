#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"
using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKNormalizeImageTest.cpp(defaults): read
// Ramp-Up-Short.nrrd (int16) through OUR reader, run OUR NormalizeImageFilter (AlwaysFloat64 output, no params),
// then compare our float64 output to ITK's committed baseline .nrrd (type: double, SAME reader) at the ITK test's
// tolerance 0.0001 via ip_golden::CompareImages. The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an
// RGB/vector input (plan Sec.5; it is also disabled in the ITK source).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::NormalizeImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][NormalizeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Up-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(NormalizeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NormalizeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NormalizeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NormalizeImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_NormalizeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
