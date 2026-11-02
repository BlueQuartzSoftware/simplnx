#include "ItkGoldenTestUtils.hpp"

#include "ImageProcessing/Filters/SigmoidImageFilter.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSigmoidImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SigmoidImageFilter with the ITK case's params
// (defaults -- Alpha/Beta/OutputMin/OutputMax unset), then compare the output md5 with ITK's committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SigmoidImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][SigmoidImageFilter]")
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
  args.insertOrAssign(SigmoidImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SigmoidImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SigmoidImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SigmoidImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("SigmoidImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "c9222b9c9150e0d7a07e9de184c10167"));
  REQUIRE(ourMd5 == "c9222b9c9150e0d7a07e9de184c10167");

  UnitTest::CheckArraysInheritTupleDims(ds);
}
