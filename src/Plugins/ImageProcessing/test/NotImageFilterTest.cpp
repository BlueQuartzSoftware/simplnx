#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MedianImageFilter.hpp"
#include "ImageProcessing/Filters/NotImageFilter.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"

using namespace nx::core;

TEST_CASE("ImageProcessing::NotImageFilter: Legacy parity", "[ImageProcessing][NotImageFilter]")
{
  ip_test::RunLegacyParity<NotImageFilter, int32>(*Uuid::FromString("6d67ad50-7e89-4678-85db-0b3247a2cfb9"), [](Arguments&) {}, 12, 0.0, 1.0);
}

TEST_CASE("ImageProcessing::NotImageFilter: full-overwrite preflight defers OOC zero fill", "[ImageProcessing][NotImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<int32>(dataStructure, /*dimension=*/8, /*start=*/0.0, /*step=*/1.0);
  const DataPath geometryPath = inputPath.getParent().getParent();
  const auto requireCreateAction = [](const IFilter::PreflightResult& preflightResult) -> const CreateArrayAction& {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
    const auto* action = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
    REQUIRE(action != nullptr);
    return *action;
  };

  NotImageFilter notFilter;
  Arguments notArgs = notFilter.getDefaultArguments();
  notArgs.insertOrAssign(NotImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  notArgs.insertOrAssign(NotImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  notArgs.insertOrAssign(NotImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Not Output"));
  const auto notPreflight = notFilter.preflight(dataStructure, notArgs);
  const CreateArrayAction& notAction = requireCreateAction(notPreflight);
  REQUIRE(notAction.initializationMode() == DataStoreInitializationMode::DeferredZeroFill);

  MedianImageFilter medianFilter;
  Arguments medianArgs = medianFilter.getDefaultArguments();
  medianArgs.insertOrAssign(MedianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  medianArgs.insertOrAssign(MedianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  medianArgs.insertOrAssign(MedianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Median Output"));
  const auto medianPreflight = medianFilter.preflight(dataStructure, medianArgs);
  const CreateArrayAction& medianAction = requireCreateAction(medianPreflight);
  REQUIRE(medianAction.initializationMode() == DataStoreInitializationMode::Default);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKNotImageTest.cpp(defaults): read STAPLE1.png
// (uint8) through OUR ITK-free reader, run OUR NotImageFilter (SameAsInput uint8, no params), then (B) live-ITK parity
// FIRST (bit-exact, integer) and (A) md5-validity-first. See SquareImageFilterTest.cpp for the ordering rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::NotImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][NotImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("STAPLE1.png"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(NotImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NotImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NotImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NotImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, bit-exact (integer).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<NotImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(NotImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 0.0);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("NotImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "2004dccdb2d68b953fd858a5b6a37d35"));
  REQUIRE(ourMd5 == "2004dccdb2d68b953fd858a5b6a37d35");

  UnitTest::CheckArraysInheritTupleDims(ds);
}
