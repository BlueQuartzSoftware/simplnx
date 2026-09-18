#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/RescaleIntensityImageFilter.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::RescaleIntensityImageFilter: Legacy parity", "[ImageProcessing][RescaleIntensityImageFilter]")
{
  ip_test::RunLegacyParity<RescaleIntensityImageFilter>(
      *Uuid::FromString("f08ea34d-9ad8-456c-a81b-9b3790b29379"),
      [](Arguments& a) {
        a.insertOrAssign(RescaleIntensityImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
        a.insertOrAssign(RescaleIntensityImageFilter::k_OutputMaximum_Key, std::make_any<float64>(1000.0));
      },
      12, -300.0, 1.0);
}

// Regression for the narrow-integer output-cast UB: an int8 image with the DEFAULT output range [0, 255]. 255 is
// not representable in int8, so the pre-fix code computed static_cast<int8>(255.0) (UB, typically -1), the clamp
// bounds inverted, and the whole image collapsed to ~0. The fix clamps in the double domain then saturates to
// int8's [-128, 127], so the input maximum maps to a saturated 127 (NOT 0) and every output is a valid int8.
TEST_CASE("ImageProcessing::RescaleIntensityImageFilter: int8 output saturates (no out-of-range cast UB)", "[ImageProcessing][RescaleIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<int8>(ds, 4, 0.0, 1.0); // 4x4x4 = 64 values, 0..63 (fits int8)

  RescaleIntensityImageFilter filter;
  Arguments args;
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMinimum_Key, std::make_any<float64>(0.0));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMaximum_Key, std::make_any<float64>(255.0)); // exceeds int8 range on purpose
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int8>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(0) == static_cast<int8>(0));    // input min -> output min
  REQUIRE(outStore.getValue(63) == static_cast<int8>(127)); // input max maps to 255, saturated to int8 max (NOT 0)
  bool allInRange = true;
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    if(outStore.getValue(i) < static_cast<int8>(0) || outStore.getValue(i) > static_cast<int8>(127))
    {
      allInRange = false;
      break;
    }
  }
  REQUIRE(allInRange);
}

// Constant-input branches of makeMapOp (RescaleIntensityImageFilter.cpp): when the input min == max the linear
// map is degenerate. Two sub-branches: a constant NON-ZERO image (inMax != 0 -> scale = (outMax-outMin)/inMax)
// and an ALL-ZEROS image (inMin == inMax == 0 -> the implicit scale = 0). Both must collapse the whole image to
// OutputMinimum, matching ITK's RescaleIntensity (scale = 1, shift = outMin - inMin -> every voxel -> outMin).
TEST_CASE("ImageProcessing::RescaleIntensityImageFilter: constant input maps to OutputMinimum", "[ImageProcessing][RescaleIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr float64 k_OutputMinimum = 5.0;
  constexpr float64 k_OutputMaximum = 255.0;
  double constantValue = 42.0;
  SECTION("constant non-zero image (inMax != 0 branch)")
  {
    constantValue = 42.0;
  }
  SECTION("all-zeros image (implicit scale==0 branch)")
  {
    constantValue = 0.0;
  }

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 4, constantValue, 0.0); // step 0 -> constant image (inMin == inMax)

  RescaleIntensityImageFilter filter;
  Arguments args;
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMinimum_Key, std::make_any<float64>(k_OutputMinimum));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputMaximum_Key, std::make_any<float64>(k_OutputMaximum));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i << " constantValue=" << constantValue);
    REQUIRE(outStore.getValue(i) == static_cast<float32>(k_OutputMinimum));
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKRescaleIntensityImageTest.cpp(3d): read RA-Float.nrrd
// (float32) through OUR reader, run OUR RescaleIntensityImageFilter (SameAsInput float32, no params), then:
//   (A) DURABLE golden -- compare our output to ITK's committed baseline .nrrd (type: float, SAME reader) at the ITK
//       test's tolerance 1e-8 via ip_golden::CompareImages.
//   (B) LIVE-ITK parity -- legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
// Pinned ForceInCore: the legacy ITK filter bad_casts an OOC store (reading via OUR reader is OOC-safe).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RescaleIntensityImageFilter: ITK real-image golden (3d)", "[ImageProcessing][ItkGolden][RescaleIntensityImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(RescaleIntensityImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(RescaleIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  RescaleIntensityImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 1e-8.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_RescaleIntensityImageFilter_3d.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 1e-8);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, tolerant (1e-4).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<RescaleIntensityImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(RescaleIntensityImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 1.0e-4);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
