#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SquareImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::SquareImageFilter: Legacy parity", "[ImageProcessing][SquareImageFilter]")
{
  ip_test::RunLegacyParity<SquareImageFilter, float32>(*Uuid::FromString("385ca853-626c-43bb-ae86-db8d8b72693b"), [](Arguments&) {}, 12, -500.0, 1.0);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK.
TEST_CASE("ImageProcessing::SquareImageFilter: Legacy parity (float64)", "[ImageProcessing][SquareImageFilter]")
{
  ip_test::RunLegacyParity<SquareImageFilter, float64>(*Uuid::FromString("385ca853-626c-43bb-ae86-db8d8b72693b"), [](Arguments&) {}, 12, -500.0, 1.0);
}

// PART A domain edge (documents ITK-faithful behavior): the op is { double d = x; return static_cast<U>(d*d); }.
// For x = 1e20 (representable in float32), d*d = 1e40 which exceeds FLT_MAX (~3.4e38), so the narrowing cast to
// float32 yields +inf. ITK's Sqr functor squares the same way, so this pins the observed overflow to +inf.
TEST_CASE("ImageProcessing::SquareImageFilter: square near sqrt(FLT_MAX) overflows to +inf (domain edge)", "[ImageProcessing][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {1.0e20f});
  const DataPath output = ip_test::RunPointwiseFilter<SquareImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // (1e20)^2 = 1e40 > FLT_MAX -> +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSquareImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SquareImageFilter (defaults), then:
//   (B) LIVE-ITK parity FIRST -- run the legacy ITK filter on the SAME input into the SAME ds and compare via
//       ip_golden::CompareImages at the float-math class (1e-4). Done before (A) so the md5-validity-first check
//       has the live-ITK verdict even on md5 skew (plan Sec.4).
//   (A) DURABLE golden -- our output's md5 must EQUAL ITK's committed hash. A match confirms the durable golden
//       byte-for-byte (no re-freeze needed). Pinned ForceInCore (legacy ITK filter bad_casts an OOC store).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SquareImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][SquareImageFilter]")
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
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SquareImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<SquareImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("SquareImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "314065b457b66e102b9cafd7c49be6b3"));
  REQUIRE(ourMd5 == "314065b457b66e102b9cafd7c49be6b3");

  UnitTest::CheckArraysInheritTupleDims(ds);
}
