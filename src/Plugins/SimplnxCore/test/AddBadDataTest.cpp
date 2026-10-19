#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>

#include "SimplnxCore/Filters/AddBadDataFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ImageGeom = DataPath({Constants::k_SmallIN100});
const DataPath k_CellDataAM = k_ImageGeom.createChildPath(Constants::k_EbsdScanData);
const DataPath k_EuclideanDistances = k_CellDataAM.createChildPath("GBManhattanDistances");
} // namespace

TEST_CASE("SimplnxCore::AddBadDataFilter: Valid Filter Execution", "[SimplnxCore][AddBadDataFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "add_bad_data_test.tar.gz", "add_bad_data_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_test.dream3d", unit_test::k_TestFilesDir)));

  // Create the filter arguments for the calculation.
  AddBadDataFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_baseline.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  args.insertOrAssign(AddBadDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489ULL));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(k_EuclideanDistances));
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeom));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellDataAM, Constants::k_SmallIN100);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AddBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][AddBadDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "AddBadDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "AddBadDataFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<AddBadDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(AddBadDataFilter::k_UseSeed_Key) == true);
      CHECK(args.value<uint64>(AddBadDataFilter::k_SeedValue_Key) == 5);
      CHECK(args.value<bool>(AddBadDataFilter::k_PoissonNoise_Key) == true);
      CHECK(args.value<float32>(AddBadDataFilter::k_PoissonVolFraction_Key) == 2.5f);
      CHECK(args.value<bool>(AddBadDataFilter::k_BoundaryNoise_Key) == true);
      CHECK(args.value<float32>(AddBadDataFilter::k_BoundaryVolFraction_Key) == 2.5f);
      CHECK(args.value<DataPath>(AddBadDataFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("SimplnxCore::AddBadDataFilter: Geometry cell tuple count", "[SimplnxCore][AddBadDataFilter][TupleCount]")
{
  using Filter = AddBadDataFilter;
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({2, 3, 2});
  geometry->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 3, 2}, geometry->getId());
  REQUIRE(cellAM != nullptr);
  geometry->setCellData(*cellAM);

  // Matching arrays remain valid outside the geometry's Cell Attribute Matrix, with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAM = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAM != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAM->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  Filter filter;
  Arguments args;
  args.insertOrAssign(Filter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(Filter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(Filter::k_PoissonNoise_Key, std::make_any<bool>(true));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& error = result.outputActions.errors()[0];
    REQUIRE(error.code == -76238);
    REQUIRE(error.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(error.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(error.message.find("'Image' has 12 cells") != std::string::npos);
  }
}
