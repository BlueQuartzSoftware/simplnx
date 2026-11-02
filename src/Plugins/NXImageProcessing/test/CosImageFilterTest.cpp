#include "ItkGoldenTestUtils.hpp"

#include "ImageProcessing/Filters/CosImageFilter.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image baseline golden cases below. Mirrors ITKCosImageTest.cpp:
// read the ITK case's input (float32 or int16) through OUR ITK-free reader, run OUR CosImageFilter (SameAsInput,
// no params), then compare with the committed baseline @0.01 via ip_golden::CompareImages.
// -----------------------------------------------------------------------------
void RunCosItkGolden(const std::string& inputFile, const std::string& baselineFile)
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

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::CosImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][CosImageFilter]")
{
  RunCosItkGolden("RA-Slice-Float.nrrd", "BasicFilters_CosImageFilter_float.nrrd");
}

TEST_CASE("ImageProcessing::CosImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][CosImageFilter]")
{
  RunCosItkGolden("RA-Slice-Short.nrrd", "BasicFilters_CosImageFilter_short.nrrd");
}
