#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SigmoidImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::SigmoidImageFilter: Legacy parity", "[ImageProcessing][SigmoidImageFilter]")
{
  ip_test::RunLegacyParity<SigmoidImageFilter, float32>(
      *Uuid::FromString("cb9ec2b6-80d9-42e6-807b-d908bea6daea"),
      [](Arguments& args) {
        args.insertOrAssign(SigmoidImageFilter::k_Alpha_Key, std::make_any<float64>(2.0));
        args.insertOrAssign(SigmoidImageFilter::k_Beta_Key, std::make_any<float64>(10.0));
        args.insertOrAssign(SigmoidImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0));
        args.insertOrAssign(SigmoidImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
      },
      12, -50.0, 0.5);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSigmoidImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SigmoidImageFilter with the ITK case's params
// (defaults -- Alpha/Beta/OutputMin/OutputMax unset, so both our filter and the legacy ITK filter use their -- shared
// -- defaults), then (B) live-ITK parity FIRST (tolerant float, 1e-4) and (A) md5-validity-first. See
// SquareImageFilterTest.cpp for the ordering rationale.
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

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<SigmoidImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(SigmoidImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("SigmoidImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "c9222b9c9150e0d7a07e9de184c10167"));
  REQUIRE(ourMd5 == "c9222b9c9150e0d7a07e9de184c10167");

  UnitTest::CheckArraysInheritTupleDims(ds);
}
