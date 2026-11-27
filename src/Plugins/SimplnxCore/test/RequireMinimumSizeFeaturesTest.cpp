#include "FeatureRemovalTestUtils.hpp"
#include "SimplnxCore/Filters/RequireMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::unit_test;

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Small IN100 Pipeline", "[SimplnxCore][RequireMinimumSizeFeatures]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_min_size_output.tar.gz", "6_6_min_size_output.dream3d");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "6_6_min_size_input.tar.gz", "6_6_min_size_input.dream3d");

  // Load the expected feature arrays from the output exemplar.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_min_size_output.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  // Load the Small IN100 input before feature removal.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_min_size_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  const std::string kGrainData("Grain Data");
  const std::string kNumCells("NumElements");
  const std::string kExemplarDataContainer("Exemplar Data");
  const DataPath kDataContainerPath({constants::k_DataContainer});
  const DataPath kCellAttributeMatrix = kDataContainerPath.createChildPath(constants::k_CellData);
  const DataPath kCellFeatureAttributeMatrix = kDataContainerPath.createChildPath(kGrainData);
  const DataPath kFeatureIdsArrayPath = kCellAttributeMatrix.createChildPath(constants::k_FeatureIds);
  const DataPath kNumCellsPath = kCellFeatureAttributeMatrix.createChildPath(kNumCells);
  const DataPath kFeaturePhasesPath = kCellFeatureAttributeMatrix.createChildPath(constants::k_Phases);

  {
    RequireMinimumSizeFeaturesFilter filter;

    Arguments args;
    // Configure the minimum size and the feature arrays that compaction updates.
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(1));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(kDataContainerPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(kFeatureIdsArrayPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(kNumCellsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(kFeaturePhasesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare each generated feature array with its corresponding exemplar array.
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(kCellFeatureAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Snapshot the generated child paths before comparisons start.
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(kCellFeatureAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // The exemplar uses a different top-level container but the same child path.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = kExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      // An absent exemplar array means that this generated array has no reference.
      if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
      {
        continue;
      }

      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      std::cout << "Comparing: " << cellArrayPath.toString() << " <==> " << exemplarDataArrayPath.toString() << '\n';

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(),
                                 fmt::underlying(type), fmt::underlying(exemplarType))
                  << '\n';
        continue;
      }

      switch(type)
      {
      case DataType::boolean: {
        unit_test::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        unit_test::CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        unit_test::CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        unit_test::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        unit_test::CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        unit_test::CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        unit_test::CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        unit_test::CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        unit_test::CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        unit_test::CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        unit_test::CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Strict Feature Ids", "[SimplnxCore][RequireMinimumSizeFeatures][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  const DataPath geometryPath({"Strict Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const DataPath idsPath = cellPath.createChildPath("Ids");
  const DataPath otherPath = geometryPath.createChildPath("Other Cells").createChildPath("Other Values");
  const ShapeType shape{1, 1, 4};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {4}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Cells", shape, imageGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId()) != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Other Values", shape, {1}, otherPtr->getId()) != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, featuresPtr->getId()) != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Cell Feature Values", shape, {1}, cellsPtr->getId()) != nullptr);
  auto* elsewherePtr = DataGroup::Create(dataStructure, "Elsewhere");
  REQUIRE(elsewherePtr != nullptr);
  auto* externalFeaturesPtr = AttributeMatrix::Create(dataStructure, "External Features", {4}, elsewherePtr->getId());
  REQUIRE(externalFeaturesPtr != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Feature Values", {4}, {1}, externalFeaturesPtr->getId()) != nullptr);

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
    REQUIRE(unit_test::CreateTestDataArray<int32>(dataStructure, "Ungrouped Features", {4}, {1}, elsewherePtr->getId()) != nullptr);
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
    REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), elsewherePtr->getId()));
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
    REQUIRE(result.output_actions.valid());
    for(const auto& modification : result.output_actions.value().modified_actions)
    {
      CHECK(modification.modified_path.getParent() != DataPath({"Elsewhere", "Cells"}));
    }
  }
  else
  {
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().size() == 1);
    CHECK(result.output_actions.errors().front().code == expectedCode);
    CHECK(result.output_actions.errors().front().message.find(offendingPath.toString()) != std::string::npos);
    CHECK(result.output_actions.errors().front().message.find(cellPath.toString()) != std::string::npos);
    CHECK(result.output_actions.errors().front().message.find(geometryPath.toString()) != std::string::npos);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Shared Cell Paths and Selected Feature Matrix", "[SimplnxCore][RequireMinimumSizeFeatures][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  const bool useAliasForIds = GENERATE(false, true);
  CAPTURE(useAliasForIds);
  DataStructure dataStructure;
  const DataPath geometryPath({"Alias Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath aliasCellPath({"Earlier Parent", "Cells"});
  const DataPath featurePath = geometryPath.createChildPath("Features");
  const ShapeType shape{1, 1, 4};
  auto* earlierParentPtr = DataGroup::Create(dataStructure, "Earlier Parent");
  REQUIRE(earlierParentPtr != nullptr);
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), earlierParentPtr->getId()));
  REQUIRE(imageGeomPtr->getCellDataPath() == cellPath);
  auto* idsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId());
  auto* valuesPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Values", shape, {1}, cellsPtr->getId());
  auto* ignoredPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Ignored", shape, {1}, cellsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(ignoredPtr != nullptr);
  const std::vector<int32> originalIds{1, 2, 1, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*idsPtr)[index] = originalIds[index];
    (*valuesPtr)[index] = static_cast<int32>(10 + index);
    (*ignoredPtr)[index] = static_cast<int32>(100 + index);
  }

  // The selected array has another parent; only the selected Feature Attribute Matrix is compacted.
  auto* featureValuesPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Feature Values", {3}, {1}, earlierParentPtr->getId());
  auto* externalMarkerPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "External Marker", {3}, {1}, earlierParentPtr->getId());
  REQUIRE(featureValuesPtr != nullptr);
  REQUIRE(externalMarkerPtr != nullptr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {3}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(featureValuesPtr->getId(), featuresPtr->getId()));
  auto* markerPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Marker", {3}, {1}, featuresPtr->getId());
  REQUIRE(markerPtr != nullptr);
  for(usize index = 0; index < 3; index++)
  {
    (*markerPtr)[index] = static_cast<int32>(11 * index);
    (*externalMarkerPtr)[index] = static_cast<int32>(90 + index);
  }
  (*featureValuesPtr)[0] = 0;
  (*featureValuesPtr)[1] = 3;
  (*featureValuesPtr)[2] = 1;

  RequireMinimumSizeFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>((useAliasForIds ? aliasCellPath : cellPath).createChildPath("Ids")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(featurePath.createChildPath("Feature Values")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(2));
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  bool reportsValues = false;
  for(const auto& modification : preflight.output_actions.value().modified_actions)
  {
    CHECK(modification.modified_path.getParent() != aliasCellPath);
    reportsValues = reportsValues || modification.modified_path == cellPath.createChildPath("Values");
  }
  CHECK(reportsValues);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  for(usize index = 0; index < 4; index++)
  {
    CHECK((*idsPtr)[index] == 1);
    if(index != 1)
    {
      CHECK((*valuesPtr)[index] == static_cast<int32>(10 + index));
    }
  }
  CHECK(((*valuesPtr)[1] == 10 || (*valuesPtr)[1] == 12));
  CHECK(featuresPtr->getNumberOfTuples() == 2);
  REQUIRE(markerPtr->getNumberOfTuples() == 2);
  CHECK((*markerPtr)[0] == 0);
  CHECK((*markerPtr)[1] == 11);
  REQUIRE(externalMarkerPtr->getNumberOfTuples() == 3);
  for(usize index = 0; index < 3; index++)
  {
    CHECK((*externalMarkerPtr)[index] == static_cast<int32>(90 + index));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RequireMinimumSizeFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RequireMinimumSizeFeaturesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: Preflight Stale Arrays", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][preflight]")
{
  unit_test::LoadPlugins();
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_min_size_input.tar.gz", "6_6_min_size_input.dream3d");
  DataStructure dataStructure = unit_test::LoadDataStructure(fs::path(fmt::format("{}/6_6_min_size_input.dream3d", unit_test::k_TestFilesDir)));
  const DataPath imageGeomPath({constants::k_DataContainer});
  const DataPath featureAMPath = imageGeomPath.createChildPath("Grain Data");
  const DataPath numCellsPath = featureAMPath.createChildPath("NumElements");
  const auto* featureAmPtr = dataStructure.getDataAs<AttributeMatrix>(featureAMPath);
  REQUIRE(featureAmPtr != nullptr);
  REQUIRE(Int32NeighborListType::Create(dataStructure, "StaleArrayTestNeighbors", featureAmPtr->getShape(), featureAmPtr->getId()) != nullptr);

  RequireMinimumSizeFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(imageGeomPath.createChildPath(constants::k_CellData).createChildPath(constants::k_FeatureIds)));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(numCellsPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  usize staleArrayCount = 0;
  for(const auto& value : preflightResult.output_values)
  {
    CHECK(value.name != "Feature Data Modification Warning");
    if(value.name == "Stale Arrays")
    {
      ++staleArrayCount;
      CHECK(value.value.find(numCellsPath.toString()) != std::string::npos);
      for(const auto& [identifier, object] : *featureAmPtr)
      {
        const std::string path = featureAMPath.createChildPath(object->getName()).toString();
        if(dynamic_cast<const IDataArray*>(object.get()) != nullptr)
        {
          CHECK(value.value.find(path) != std::string::npos);
        }
        else if(dynamic_cast<const INeighborList*>(object.get()) != nullptr)
        {
          CHECK(value.value.find(path) == std::string::npos);
        }
      }
    }
  }
  CHECK(staleArrayCount == 1);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
