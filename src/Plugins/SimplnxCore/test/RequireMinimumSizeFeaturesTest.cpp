#include "FeatureRemovalTestUtils.hpp"
#include "SimplnxCore/Filters/RequireMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

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
