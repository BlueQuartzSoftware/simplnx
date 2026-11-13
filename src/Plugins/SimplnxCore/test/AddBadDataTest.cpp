#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IParameter.hpp"
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
  using FilterType = AddBadDataFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 3, 2}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Matching arrays remain valid outside the geometry's Cell Attribute Matrix, with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterType::k_PoissonNoise_Key, std::make_any<bool>(true));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -76238);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::AddBadData component contract", "[SimplnxCore][AddBadDataFilter][ComponentContract]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  const usize components = GENERATE(usize{1}, usize{3});
  CAPTURE(components);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  const ShapeType tupleShape{2, 3, 4};
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  const DataPath distancesPath({"Image", "Cell Data", "Distances"});
  auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, distancesPath, tupleShape, ShapeType{components});
  REQUIRE(store != nullptr);
  auto* distancesPtr = Int32Array::Create(dataStructure, "Distances", store, cellAMPtr->getId());
  REQUIRE(distancesPtr != nullptr);
  REQUIRE(imageGeomPtr->getNumberOfCells() == 24);
  REQUIRE(distancesPtr->getNumberOfTuples() == 24);
  REQUIRE(distancesPtr->getNumberOfTuples() == imageGeomPtr->getNumberOfCells());
  REQUIRE(distancesPtr->getNumberOfComponents() == components);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  const AddBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(distancesPath));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(0.5F));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryVolFraction_Key, std::make_any<float32>(0.5F));

  const auto result = filter.preflight(dataStructure, args);
  if(components == 1)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    REQUIRE(result.outputActions.errors()[0].code == FilterParameter::Constants::k_Validate_TupleShapeValue);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AddBadData: rejects nonnumeric input arrays", "[SimplnxCore][AddBadDataFilter]")
{
  DataStructure dataStructure;
  auto* image = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 1, 1});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 2}, image->getId());
  REQUIRE(cells != nullptr);
  image->setCellData(*cells);
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Input", {2}, {"one", "two"}) != nullptr);

  const AddBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input"})));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  IFilter::PreflightResult preflight;
  REQUIRE_NOTHROW(preflight = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.errors().front().code == -76240);
  REQUIRE(preflight.outputActions.errors().front().message.find("Input") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AddBadDataFilter: Reject nonnumeric cell arrays", "[SimplnxCore][AddBadDataFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool useStrings = GENERATE(true, false);
  CAPTURE(useStrings);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({1, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 1, 1}, imageGeom->getId());
  REQUIRE(cellAM != nullptr);
  imageGeom->setCellData(*cellAM);
  REQUIRE(Int32Array::Create(dataStructure, "Distances", std::make_shared<DataStore<int32>>(ShapeType{1, 1, 1}, ShapeType{1}, 0), cellAM->getId()) != nullptr);
  if(useStrings)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, "Unsupported", cellAM->getShape(), {"label"}, cellAM->getId()) != nullptr);
  }
  else
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, "Unsupported", cellAM->getShape(), cellAM->getId()) != nullptr);
  }

  AddBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "Distances"})));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(1.0F));
  args.insertOrAssign(AddBadDataFilter::k_UseSeed_Key, std::make_any<bool>(true));

  // Type-contract oracle: zeroing numeric components is undefined for strings and variable-length lists.
  // Reject either type in preflight, before execution can cast it to IDataArray.
  const auto preflight = filter.preflight(dataStructure, args);
  CHECK(preflight.outputActions.invalid());
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(execute.result);
  REQUIRE(execute.result.errors().size() == 1);
  CHECK(execute.result.errors()[0].code == -76239);
  CHECK(execute.result.errors()[0].message.find("Image/Cell Data/Unsupported") != std::string::npos);
  if(preflight.outputActions.invalid())
  {
    REQUIRE(preflight.outputActions.errors().size() == 1);
    CHECK(preflight.outputActions.errors()[0].code == -76239);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
