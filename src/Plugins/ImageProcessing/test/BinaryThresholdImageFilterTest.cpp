#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryThresholdImageFilter.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image md5 golden cases below. Mirrors ITKBinaryThresholdImageTest.cpp:
// read RA-Short.nrrd (int16) through OUR ITK-free reader, run OUR BinaryThresholdImageFilter (AlwaysUInt8 output)
// with the ITK case's params, then compare the output md5 with ITK's committed hash (plan Sec.4).
// -----------------------------------------------------------------------------
void RunBinaryThresholdItkGolden(const std::function<void(Arguments&)>& setParams, const std::string& committedMd5)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(BinaryThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(BinaryThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(BinaryThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  BinaryThresholdImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("BinaryThresholdImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::BinaryThresholdImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][BinaryThresholdImageFilter]")
{
  RunBinaryThresholdItkGolden([](Arguments&) {}, "dbd0ea7d6f16bb93e9c688cb0f1bfd85");
}

TEST_CASE("ImageProcessing::BinaryThresholdImageFilter: ITK real-image golden (NarrowThreshold)", "[ImageProcessing][ItkGolden][BinaryThresholdImageFilter]")
{
  RunBinaryThresholdItkGolden(
      [](Arguments& args) {
        args.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(10.0));
        args.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(100.0));
        args.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(255));
        args.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(0));
      },
      "fc4ce029c088096a69d033ccc5bc1ae2");
}

// Matches ITK's BinaryThresholdImageFilter, which throws when LowerThreshold > UpperThreshold, and the filter's
// own parameter documentation, which promises the same. The bad ordering must be rejected at preflight; equal and
// ascending thresholds must preflight cleanly.
TEST_CASE("ImageProcessing::BinaryThresholdImageFilter: Preflight rejects Lower > Upper", "[ImageProcessing][BinaryThresholdImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildRampImage<float32>(ds, 4, 0.0, 1.0);
  const DataPath geom({"Image Geometry"});

  BinaryThresholdImageFilter filter;
  auto makeArgs = [&](float64 lower, float64 upper) {
    Arguments args;
    args.insertOrAssign(BinaryThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
    args.insertOrAssign(BinaryThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
    args.insertOrAssign(BinaryThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(lower));
    args.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(upper));
    args.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(1));
    args.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(0));
    return args;
  };

  // Lower > Upper -> rejected at preflight.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(500.0, 100.0)).outputActions.valid());
  // Lower == Upper and Lower < Upper -> preflight cleanly.
  auto equalThresholdResult = filter.preflight(ds, makeArgs(250.0, 250.0));
  SIMPLNX_RESULT_REQUIRE_VALID(equalThresholdResult.outputActions);
  auto orderedThresholdResult = filter.preflight(ds, makeArgs(100.0, 500.0));
  SIMPLNX_RESULT_REQUIRE_VALID(orderedThresholdResult.outputActions);
}
