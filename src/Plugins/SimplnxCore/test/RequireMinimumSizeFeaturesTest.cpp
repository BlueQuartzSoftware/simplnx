#include "FeatureRemovalTestUtils.hpp"
#include "SimplnxCore/Filters/RequireMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

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
  namespace Fixture = FeatureRemovalTest::StrictFeatureIds;
  const auto parents = Fixture::BuildInput<int32>(dataStructure);

  RequireMinimumSizeFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(Fixture::k_GeometryPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(Fixture::k_CellPath.createChildPath("Ids")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(Fixture::k_FeatureValuesPath));

  int32 expectedCode = 0;
  DataPath offendingPath;
  SECTION("Correct Cell and Feature Attribute Matrices")
  {
  }
  SECTION("Feature Ids in a second Attribute Matrix with the same tuple shape")
  {
    offendingPath = Fixture::k_OtherValuesPath;
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
    REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ungrouped Features", {4}, {1}, parents.Elsewhere) != nullptr);
    offendingPath = DataPath({"Elsewhere", "Ungrouped Features"});
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5563;
  }
  SECTION("Feature array in the Cell Attribute Matrix")
  {
    offendingPath = Fixture::k_CellPath.createChildPath("Cell Feature Values");
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(offendingPath));
    expectedCode = -5563;
  }
  SECTION("Feature Ids through a shared Cell Attribute Matrix")
  {
    REQUIRE(dataStructure.setAdditionalParent(parents.Cells, parents.Elsewhere));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere", "Cells", "Ids"})));
  }
  SECTION("Read-only Feature Phases in another Attribute Matrix")
  {
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(Fixture::k_OtherValuesPath));
  }
  const auto result = filter.preflight(dataStructure, args);
  Fixture::CheckPreflight(result, expectedCode, offendingPath);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeatures: Shared Cell Paths and Selected Feature Matrix", "[SimplnxCore][RequireMinimumSizeFeatures][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  const bool useAliasForIds = GENERATE(false, true);
  CAPTURE(useAliasForIds);
  DataStructure dataStructure;
  namespace Fixture = FeatureRemovalTest::SharedCellPaths;
  Fixture::BuildInput<int32>(dataStructure, {0, 3, 1});

  RequireMinimumSizeFeaturesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(Fixture::k_GeometryPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>((useAliasForIds ? Fixture::k_AliasCellPath : Fixture::k_CellPath).createChildPath("Ids")));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(Fixture::k_FeatureValuesPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(2));
  const auto preflight = filter.preflight(dataStructure, args);
  Fixture::CheckModifiedPaths(preflight);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  Fixture::CheckCompaction(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][BackwardsCompatibility]")
{
  const auto version = GENERATE("6_4", "6_5");
  CAPTURE(version);
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / version / "RequireMinimumSizeFeaturesFilter.json";
  const auto args = UnitTest::LoadSIMPLConversionArguments(fixturePath, FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid);
  CHECK(args.value<int64>(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key) == 5);
  CHECK(args.value<bool>(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key) == true);
  CHECK(args.value<int32>(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key) == 5);
  CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
  CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: Preflight Stale Arrays", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_min_size_input.tar.gz", "6_6_min_size_input.dream3d");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_min_size_input.dream3d", unit_test::k_TestFilesDir)));
  const DataPath imageGeomPath({Constants::k_DataContainer});
  const DataPath featureAMPath = imageGeomPath.createChildPath("Grain Data");
  const DataPath numCellsPath = featureAMPath.createChildPath("NumElements");
  const auto* featureAM = dataStructure.getDataAs<AttributeMatrix>(featureAMPath);
  REQUIRE(featureAM != nullptr);
  REQUIRE(Int32NeighborList::Create(dataStructure, "StaleArrayTestNeighbors", featureAM->getShape(), featureAM->getId()) != nullptr);

  RequireMinimumSizeFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(imageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds)));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(numCellsPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  for(const auto& value : preflightResult.outputValues)
  {
    CHECK(value.name != "Feature Data Modification Warning");
  }
  const auto report = FeatureRemovalTest::GetStaleArrayReport(preflightResult);
  CHECK(report.find(numCellsPath.toString()) != std::string::npos);
  for(const auto& [identifier, object] : *featureAM)
  {
    const std::string path = featureAMPath.createChildPath(object->getName()).toString();
    if(dynamic_cast<const IDataArray*>(object.get()) != nullptr)
    {
      CHECK(report.find(path) != std::string::npos);
    }
    else if(dynamic_cast<const INeighborList*>(object.get()) != nullptr)
    {
      CHECK(report.find(path) == std::string::npos);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
