#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/InvertIntensityImageFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <cmath>
#include <limits>

using namespace nx::core;

TEST_CASE("ImageProcessing::InvertIntensityImageFilter: Legacy parity", "[ImageProcessing][InvertIntensityImageFilter]")
{
  ip_test::RunLegacyParity<InvertIntensityImageFilter, float32>(
      *Uuid::FromString("9958d587-5698-4ea5-b8ea-fb71428b5d02"), [](Arguments& args) { args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(255.0)); }, 12, 0.0, 1.0);
}

// Regression for the narrow-integer output cast (same class as the RescaleIntensity int8 case): Maximum - value
// can exceed the int8 range, which was undefined pre-fix. With Maximum=150 on an int8 ramp 0..63, the result
// 150-value straddles 127: bright voxels saturate to the int8 max (127) and the rest stay valid int8.
TEST_CASE("ImageProcessing::InvertIntensityImageFilter: int8 output saturates (no out-of-range cast UB)", "[ImageProcessing][InvertIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<int8>(ds, 4, 0.0, 1.0); // 64 values, 0..63

  InvertIntensityImageFilter filter;
  Arguments args;
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(InvertIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(150.0));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int8>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(0) == static_cast<int8>(127)); // 150-0=150 -> saturated to int8 max
  REQUIRE(outStore.getValue(63) == static_cast<int8>(87)); // 150-63=87 -> in range, exact
  bool allInRange = true;
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    if(outStore.getValue(i) < static_cast<int8>(-128) || outStore.getValue(i) > static_cast<int8>(127))
    {
      allInRange = false;
      break;
    }
  }
  REQUIRE(allInRange);
}

TEST_CASE("ImageProcessing::InvertIntensityImageFilter: fractional maximum preserves Float64 semantics", "[ImageProcessing][InvertIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<int8>(ds, 2, 0.0, 1.0);
  InvertIntensityImageFilter filter;
  Arguments args;
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(InvertIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(0.5));
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& output = ds.getDataRefAs<DataArray<int8>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(output.getValue(0) == 0);  // 0.5 truncates toward zero
  REQUIRE(output.getValue(1) == 0);  // -0.5 truncates toward zero
  REQUIRE(output.getValue(2) == -1); // -1.5 truncates toward zero
}

TEST_CASE("ImageProcessing::InvertIntensityImageFilter: extreme integral maximum uses safe fallback", "[ImageProcessing][InvertIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<int32>(ds, 2, 0.0, 1.0);
  ds.getDataRefAs<DataArray<int32>>(inputPath).getDataStoreRef().setValue(0, std::numeric_limits<int32>::lowest());
  InvertIntensityImageFilter filter;
  Arguments args;
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(InvertIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(InvertIntensityImageFilter::k_Maximum_Key, std::make_any<float64>(std::ldexp(1.0, 63) - 2048.0));
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& output = ds.getDataRefAs<DataArray<int32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(output.getValue(0) == std::numeric_limits<int32>::max());
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKInvertIntensityImageTest.cpp(3d): read
// RA-Short.nrrd (int16) through OUR ITK-free reader, run OUR InvertIntensityImageFilter (SameAsInput int16, defaults --
// Maximum unset, so both filters use their shared default), then (B) live-ITK parity FIRST (bit-exact, integer) and
// (A) md5-validity-first. See SquareImageFilterTest.cpp for the ordering rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::InvertIntensityImageFilter: ITK real-image golden (3d)", "[ImageProcessing][ItkGolden][InvertIntensityImageFilter]")
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
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(InvertIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(InvertIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  InvertIntensityImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, bit-exact (integer).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<InvertIntensityImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(InvertIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 0.0);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("InvertIntensityImageFilter 3d md5: ours='{}' committed='{}'", ourMd5, "76765a57f26a7979f33efc8ed9801a55"));
  REQUIRE(ourMd5 == "76765a57f26a7979f33efc8ed9801a55");

  UnitTest::CheckArraysInheritTupleDims(ds);
}
