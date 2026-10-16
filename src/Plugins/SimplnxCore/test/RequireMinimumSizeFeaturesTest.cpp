#include "FeatureRemovalTestUtils.hpp"
#include "SimplnxCore/Filters/RequireMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Small IN100 Pipeline", "[SimplnxCore][RequireMinimumSizeFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_min_size_output.tar.gz", "6_6_min_size_output.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "6_6_min_size_input.tar.gz", "6_6_min_size_input.dream3d");

  // Load the expected feature arrays from the output exemplar.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_min_size_output.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Load the Small IN100 input before feature removal.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_min_size_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_GrainData("Grain Data");
  const std::string k_NumCells("NumElements");
  const std::string k_ExemplarDataContainer("Exemplar Data");
  const DataPath k_DataContainerPath({Constants::k_DataContainer});
  const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath(Constants::k_CellData);
  const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_GrainData);
  const DataPath k_FeatureIdsArrayPath = k_CellAttributeMatrix.createChildPath(Constants::k_FeatureIds);
  const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumCells);
  const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(Constants::k_Phases);

  {
    RequireMinimumSizeFeaturesFilter filter;

    Arguments args;
    // Configure the minimum size and the feature arrays that compaction updates.
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(1));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare each generated feature array with its corresponding exemplar array.
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellFeatureAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Snapshot the generated child paths before comparisons start.
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellFeatureAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // The exemplar uses a different top-level container but the same child path.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      // An absent exemplar array means that this generated array has no reference.
      if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
      {
        continue;
      }

      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      std::cout << "Comparing: " << cellArrayPath.toString() << " <==> " << exemplarDataArrayPath.toString() << std::endl;

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(),
                                 fmt::underlying(type), fmt::underlying(exemplarType))
                  << std::endl;
        continue;
      }

      switch(type)
      {
      case DataType::boolean: {
        UnitTest::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        UnitTest::CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        UnitTest::CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        UnitTest::CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        UnitTest::CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        UnitTest::CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        UnitTest::CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        UnitTest::CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        UnitTest::CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        UnitTest::CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/7_0_min_size_output.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Strict Feature Ids", "[SimplnxCore][RequireMinimumSizeFeatures][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath geometryPath({"Strict Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const DataPath idsPath = cellPath.createChildPath("Ids");
  const DataPath otherPath = geometryPath.createChildPath("Other Cells").createChildPath("Other Values");
  const ShapeType shape{1, 1, 4};
  auto* geometry = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({4, 1, 1});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", shape, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(*cells);
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {4}, geometry->getId());
  REQUIRE(features != nullptr);
  auto* other = AttributeMatrix::Create(dataStructure, "Other Cells", shape, geometry->getId());
  REQUIRE(other != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cells->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Other Values", shape, {1}, other->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, features->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Cell Feature Values", shape, {1}, cells->getId()) != nullptr);
  auto* elsewhere = DataGroup::Create(dataStructure, "Elsewhere");
  REQUIRE(elsewhere != nullptr);
  auto* externalFeatures = AttributeMatrix::Create(dataStructure, "External Features", {4}, elsewhere->getId());
  REQUIRE(externalFeatures != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, externalFeatures->getId()) != nullptr);

  RequireMinimumSizeFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Feature Values")));

  int32 expectedCode = 0;
  DataPath offendingPath;
  SECTION("Correct Cell and Feature Attribute Matrices")
  {
  }
  SECTION("Feature Ids in a second Attribute Matrix with the same tuple shape")
  {
    offendingPath = otherPath;
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5562;
  }
  SECTION("Feature array outside the selected geometry")
  {
    offendingPath = DataPath({"Elsewhere", "External Features", "Feature Values"});
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5563;
  }
  SECTION("Feature array under a plain DataGroup")
  {
    REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ungrouped Features", {4}, {1}, elsewhere->getId()) != nullptr);
    offendingPath = DataPath({"Elsewhere", "Ungrouped Features"});
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5563;
  }
  SECTION("Feature array in the Cell Attribute Matrix")
  {
    offendingPath = cellPath.createChildPath("Cell Feature Values");
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5563;
  }
  SECTION("Feature Ids through a shared Cell Attribute Matrix")
  {
    REQUIRE(dataStructure.setAdditionalParent(cells->getId(), elsewhere->getId()));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere", "Cells", "Ids"})));
  }
  SECTION("Read-only Feature Phases in another Attribute Matrix")
  {
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(otherPath));
  }
  const auto result = filter.preflight(dataStructure, args);
  if(expectedCode == 0)
  {
    REQUIRE(result.outputActions.valid());
    for(const auto& modification : result.outputActions.value().modifiedActions)
    {
      CHECK(modification.modifiedPath.getParent() != DataPath({"Elsewhere", "Cells"}));
    }
  }
  else
  {
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == expectedCode);
    CHECK(result.outputActions.errors().front().message.find(offendingPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(cellPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(geometryPath.toString()) != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Shared Cell Paths and Selected Feature Matrix", "[SimplnxCore][RequireMinimumSizeFeatures][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  const bool useAliasForIds = GENERATE(false, true);
  CAPTURE(useAliasForIds);
  DataStructure dataStructure;
  const DataPath geometryPath({"Alias Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath aliasCellPath({"Earlier Parent", "Cells"});
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const ShapeType shape{1, 1, 4};
  auto* earlierParent = DataGroup::Create(dataStructure, "Earlier Parent");
  REQUIRE(earlierParent != nullptr);
  auto* geometry = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({4, 1, 1});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", shape, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(*cells);
  REQUIRE(dataStructure.setAdditionalParent(cells->getId(), earlierParent->getId()));
  REQUIRE(geometry->getCellDataPath() == cellPath);
  auto* ids = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cells->getId());
  auto* values = UnitTest::CreateTestDataArray<int32>(dataStructure, "Values", shape, {1}, cells->getId());
  auto* ignored = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ignored", shape, {1}, cells->getId());
  REQUIRE(ids != nullptr);
  REQUIRE(values != nullptr);
  REQUIRE(ignored != nullptr);
  const std::vector<int32> originalIds{1, 2, 1, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*ids)[index] = originalIds[index];
    (*values)[index] = static_cast<int32>(10 + index);
    (*ignored)[index] = static_cast<int32>(100 + index);
  }

  // The selected array has another parent; only the selected Feature Attribute Matrix is compacted.
  auto* featureValues = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Values", {3}, {1}, earlierParent->getId());
  auto* externalMarker = UnitTest::CreateTestDataArray<int32>(dataStructure, "External Marker", {3}, {1}, earlierParent->getId());
  REQUIRE(featureValues != nullptr);
  REQUIRE(externalMarker != nullptr);
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {3}, geometry->getId());
  REQUIRE(features != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(featureValues->getId(), features->getId()));
  auto* marker = UnitTest::CreateTestDataArray<int32>(dataStructure, "Marker", {3}, {1}, features->getId());
  REQUIRE(marker != nullptr);
  for(usize index = 0; index < 3; index++)
  {
    (*marker)[index] = static_cast<int32>(11 * index);
    (*externalMarker)[index] = static_cast<int32>(90 + index);
  }
  (*featureValues)[0] = 0;
  (*featureValues)[1] = 3;
  (*featureValues)[2] = 1;

  RequireMinimumSizeFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>((useAliasForIds ? aliasCellPath : cellPath).createChildPath("Ids")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Feature Values")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(2));
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool reportsValues = false;
  for(const auto& modification : preflight.outputActions.value().modifiedActions)
  {
    CHECK(modification.modifiedPath.getParent() != aliasCellPath);
    reportsValues = reportsValues || modification.modifiedPath == cellPath.createChildPath("Values");
  }
  CHECK(reportsValues);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  for(usize index = 0; index < 4; index++)
  {
    CHECK((*ids)[index] == 1);
    if(index != 1)
    {
      CHECK((*values)[index] == static_cast<int32>(10 + index));
    }
  }
  CHECK(((*values)[1] == 10 || (*values)[1] == 12));
  CHECK(features->getNumberOfTuples() == 2);
  REQUIRE(marker->getNumberOfTuples() == 2);
  CHECK((*marker)[0] == 0);
  CHECK((*marker)[1] == 11);
  REQUIRE(externalMarker->getNumberOfTuples() == 3);
  for(usize index = 0; index < 3; index++)
  {
    CHECK((*externalMarker)[index] == static_cast<int32>(90 + index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RequireMinimumSizeFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RequireMinimumSizeFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int64>(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key) == 5);
      CHECK(args.value<bool>(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key) == true);
      CHECK(args.value<int32>(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key) == 5);
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
