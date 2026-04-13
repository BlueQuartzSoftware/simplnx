#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeatureClusteringFilter.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
const std::string k_ExemplarClusteringList = "ClusteringList";
const std::string k_ExemplarRDF = "RDF";
const std::string k_ExemplarMinMaxDistances = "RDFMaxMinDistances";
const std::string k_GeneratedClusteringList = "NX_ClusteringList";
const std::string k_GeneratedRDF = "NX_RDF";
const std::string k_GeneratedMinMaxDistances = "NX_RDFMaxMinDistances";
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureClusteringFilter: Valid Filter Execution", "[SimplnxCore][ComputeFeatureClusteringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_clustering.tar.gz", "6_6_find_feature_clustering.dream3d");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_feature_clustering.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureClusteringFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(10));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Centroids)));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_ClusteringListArrayName_Key, std::make_any<std::string>(k_GeneratedClusteringList));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RDFArrayName_Key, std::make_any<std::string>(k_GeneratedRDF));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_MaxMinArrayName_Key, std::make_any<std::string>(k_GeneratedMinMaxDistances));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareNeighborLists<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_ExemplarClusteringList), k_CellFeatureDataPath.createChildPath(k_GeneratedClusteringList));
  CompareArrays<float32>(dataStructure, k_CellEnsembleAttributeMatrixPath.createChildPath(k_ExemplarRDF), k_CellEnsembleAttributeMatrixPath.createChildPath(k_GeneratedRDF));
  CompareArrays<float32>(dataStructure, k_CellEnsembleAttributeMatrixPath.createChildPath(k_ExemplarMinMaxDistances), k_CellEnsembleAttributeMatrixPath.createChildPath(k_GeneratedMinMaxDistances));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClusteringFilter: InValid Filter Execution", "[SimplnxCore][ComputeFeatureClusteringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_clustering.tar.gz", "6_6_find_feature_clustering.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_feature_clustering.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureClusteringFilter filter;
  Arguments args;

  // Mismatching cell feature data tuples case :
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(10));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_DataContainerPath.createChildPath(k_Cell_Data).createChildPath(k_Phases)));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Centroids)));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_ClusteringListArrayName_Key, std::make_any<std::string>(k_GeneratedClusteringList));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RDFArrayName_Key, std::make_any<std::string>(k_GeneratedRDF));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_MaxMinArrayName_Key, std::make_any<std::string>(k_GeneratedMinMaxDistances));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClusteringFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureClusteringFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureClusteringFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureClusteringFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureClusteringFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(ComputeFeatureClusteringFilter::k_SetRandomSeed_Key) == true);
        CHECK(args.value<uint64>(ComputeFeatureClusteringFilter::k_SeedValue_Key) == 5);
      }
      CHECK(args.value<int32>(ComputeFeatureClusteringFilter::k_NumberOfBins_Key) == 5);
      CHECK(args.value<int32>(ComputeFeatureClusteringFilter::k_PhaseNumber_Key) == 5);
      CHECK(args.value<bool>(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureClusteringFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureClusteringFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeatureClusteringFilter::k_ClusteringListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureClusteringFilter::k_MaxMinArrayName_Key) == "TestName");
    }
  }
}
