#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
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
  // The legacy RDF exemplar omits reference pairs involving sample zero (L11 / #1820).
  // Correct RDF normalization is checked by the independent closed-form test below.
  // The same seed gives the same samples, and the fix only adds reference pairs. So no bin can grow.
  // In this data set some of the 999 distances from sample zero fall inside the RDF distance range,
  // so at least one bin must shrink.
  {
    const auto& legacyRdf = dataStructure.getDataRefAs<Float32Array>(k_CellEnsembleAttributeMatrixPath.createChildPath(k_ExemplarRDF));
    const auto& generatedRdf = dataStructure.getDataRefAs<Float32Array>(k_CellEnsembleAttributeMatrixPath.createChildPath(k_GeneratedRDF));
    REQUIRE(generatedRdf.getSize() == legacyRdf.getSize());
    usize lowerBins = 0;
    for(usize i = 0; i < generatedRdf.getSize(); i++)
    {
      INFO(fmt::format("RDF index {}: generated {} legacy {}", i, generatedRdf[i], legacyRdf[i]));
      REQUIRE(generatedRdf[i] <= legacyRdf[i]);
      if(generatedRdf[i] < legacyRdf[i])
      {
        lowerBins++;
      }
    }
    REQUIRE(lowerBins > 0);
  }
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

TEST_CASE("SimplnxCore::ComputeFeatureClusteringFilter: Complete reference pair normalization", "[SimplnxCore][ComputeFeatureClusteringFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({1, 1, 1});
  geometry->setSpacing({1.0f, 1.0f, 1.0f});
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {4}, geometry->getId());
  REQUIRE(features != nullptr);
  auto* ensembles = AttributeMatrix::Create(dataStructure, "Ensembles", {2}, geometry->getId());
  REQUIRE(ensembles != nullptr);
  auto* phases = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Phases", {4}, {1}, features->getId());
  auto* centroids = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Centroids", {4}, {3}, features->getId());
  REQUIRE(phases != nullptr);
  REQUIRE(centroids != nullptr);
  phases->fill(1);
  (*phases)[0] = 0;
  centroids->fill(0.0f);
  (*centroids)[9] = 0.5f;

  ComputeFeatureClusteringFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Features", "Phases"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Features", "Centroids"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Image", "Ensembles"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RDFArrayName_Key, std::make_any<std::string>("RDF"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Closed form: a one-voxel reference puts all 1000 * 999 ordered pairs at distance zero.
  // The three feature distances are 0, 0.5, 0.5. One bin contains all six ordered pairs.
  // Its reference probability is 1, so RDF = 6 / (1 * 3 * 2) = 1, independent of seed.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(DataPath({"Image", "Ensembles", "RDF"})));
  const auto& rdf = dataStructure.getDataRefAs<Float32Array>(DataPath({"Image", "Ensembles", "RDF"}));
  REQUIRE(rdf[1] == Approx(1.0f).margin(1.0e-6f));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
