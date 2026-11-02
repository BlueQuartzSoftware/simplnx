#include "ItkGoldenTestUtils.hpp"

#include "ImageProcessing/Filters/IntensityWindowingImageFilter.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the three ITK-sourced real-image md5 golden cases below. Mirrors ITKIntensityWindowingImageTest.cpp:
// read the ITK case's input through OUR ITK-free reader, run OUR IntensityWindowingImageFilter (SameAsInput, all
// params default), then compare the output md5 with ITK's committed hash (plan Sec.4). The `2d` case (STAPLE1.png,
// defaults) leaves the image unchanged, so its hash 095f00a6... is the identity-output hash of STAPLE1 (plan Risk #1).
// -----------------------------------------------------------------------------
void RunIntensityWindowingItkGolden(const std::string& inputFile, const std::string& committedMd5)
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
  args.insertOrAssign(IntensityWindowingImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(IntensityWindowingImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(IntensityWindowingImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  IntensityWindowingImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("IntensityWindowingImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::IntensityWindowingImageFilter: ITK real-image golden (2d)", "[ImageProcessing][ItkGolden][IntensityWindowingImageFilter]")
{
  RunIntensityWindowingItkGolden("STAPLE1.png", "095f00a68a84df4396914fa758f34dcc");
}

TEST_CASE("ImageProcessing::IntensityWindowingImageFilter: ITK real-image golden (3dFloat)", "[ImageProcessing][ItkGolden][IntensityWindowingImageFilter]")
{
  RunIntensityWindowingItkGolden("RA-Float.nrrd", "199c966fabac791c758766e14df9974c");
}

TEST_CASE("ImageProcessing::IntensityWindowingImageFilter: ITK real-image golden (3dShort)", "[ImageProcessing][ItkGolden][IntensityWindowingImageFilter]")
{
  RunIntensityWindowingItkGolden("RA-Short.nrrd", "2790c2bdfeb8610821e9ec8751f95516");
}
