#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureClustering.hpp"
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
Arguments CreateBinCountClusteringFixture(DataStructure& dataStructure, int32 numberOfBins)
{
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "BinCountGeometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 4, 4});
  imageGeomPtr->setSpacing({1, 1, 1});
  imageGeomPtr->setOrigin({0, 0, 0});
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {4}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  auto* ensemblesPtr = AttributeMatrix::Create(dataStructure, "Ensembles", {2}, imageGeomPtr->getId());
  REQUIRE(ensemblesPtr != nullptr);
  const DataPath featuresPath({"BinCountGeometry", "Features"});
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, featuresPath.createChildPath("Phases"), {4}, {1});
  auto* phasesPtr = Int32Array::Create(dataStructure, "Phases", std::move(phasesStore), featuresPtr->getId());
  REQUIRE(phasesPtr != nullptr);
  const std::array<int32, 4> phases = {0, 1, 1, 1};
  for(usize index = 0; index < phases.size(); index++)
  {
    (*phasesPtr)[index] = phases[index];
  }
  auto centroidsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, featuresPath.createChildPath("Centroids"), {4}, {3});
  auto* centroidsPtr = Float32Array::Create(dataStructure, "Centroids", std::move(centroidsStore), featuresPtr->getId());
  REQUIRE(centroidsPtr != nullptr);
  const std::array<float32, 12> centroids = {0, 0, 0, 0, 0, 0, 1, 0, 0, 3, 0, 0};
  for(usize index = 0; index < centroids.size(); index++)
  {
    (*centroidsPtr)[index] = centroids[index];
  }
  auto biasedStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, featuresPath.createChildPath("Biased"), {4}, {1});
  auto* biasedPtr = BoolArray::Create(dataStructure, "Biased", std::move(biasedStore), featuresPtr->getId());
  REQUIRE(biasedPtr != nullptr);
  biasedPtr->fill(false);
  Arguments args;
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"BinCountGeometry"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(numberOfBins));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Phases")));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Centroids")));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"BinCountGeometry", "Ensembles"})));
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(featuresPath.createChildPath("Biased")));
  return args;
}
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

TEST_CASE("SimplnxCore::ComputeFeatureClustering: zero bins preflight", "[SimplnxCore][ComputeFeatureClustering]")
{
  DataStructure dataStructure;
  const Arguments args = CreateBinCountClusteringFixture(dataStructure, 0);
  const auto result = ComputeFeatureClusteringFilter{}.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -54099);
  CHECK(result.outputActions.errors().front().message == "Number of Bins for RDF must be at least 1, but was 0.");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: four bins control", "[SimplnxCore][ComputeFeatureClustering]")
{
  DataStructure dataStructure;
  Arguments args = CreateBinCountClusteringFixture(dataStructure, 4);
  const bool removeBiasedFeatures = GENERATE(false, true);
  CAPTURE(removeBiasedFeatures);
  args.insertOrAssign(ComputeFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(removeBiasedFeatures));
  ComputeFeatureClusteringFilter filter;
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath({"BinCountGeometry", "Ensembles"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath.createChildPath("RDF")));
  const auto& rdf = dataStructure.getDataRefAs<Float32Array>(outputPath.createChildPath("RDF"));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath.createChildPath("RDFMaxMinDistances")));
  const auto& maxMin = dataStructure.getDataRefAs<Float32Array>(outputPath.createChildPath("RDFMaxMinDistances"));
  REQUIRE(rdf.getSize() == 8);
  REQUIRE(maxMin.getSize() == 4);
  // Distances 1, 2, and 3 each occur twice. The last distance stays in the last bin.
  // Independent lattice-cell population enumeration gives these ordered reference-pair counts for seed 5489.
  const std::array<float32, 4> referencePairs = {175186, 53046, 292346, 46722};
  const std::array<float32, 4> observedPairs = {2, 0, 2, 2};
  for(usize bin = 0; bin < 4; bin++)
  {
    CHECK(rdf[bin] == 0);
    CHECK(rdf[4 + bin] == observedPairs[bin] / ((referencePairs[bin] / 999000.0F) * 6.0F));
  }
  const std::array<float32, 4> expectedMaxMin = {0, 0, 3, 1};
  for(usize index = 0; index < expectedMaxMin.size(); index++)
  {
    CHECK(maxMin[index] == expectedMaxMin[index]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: negative bins preflight", "[SimplnxCore][ComputeFeatureClustering]")
{
  DataStructure dataStructure;
  const Arguments args = CreateBinCountClusteringFixture(dataStructure, -1);
  const auto result = ComputeFeatureClusteringFilter{}.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -54099);
  CHECK(result.outputActions.errors().front().message == "Number of Bins for RDF must be at least 1, but was -1.");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: invalid bins execute leaves data unchanged", "[SimplnxCore][ComputeFeatureClustering]")
{
  const int32 numberOfBins = GENERATE(0, -1);
  CAPTURE(numberOfBins);
  DataStructure dataStructure;
  const Arguments args = CreateBinCountClusteringFixture(dataStructure, numberOfBins);
  const auto originalPaths = dataStructure.getAllDataPaths();
  const DataPath featuresPath({"BinCountGeometry", "Features"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featuresPath.createChildPath("Phases")));
  const auto& phases = dataStructure.getDataRefAs<Int32Array>(featuresPath.createChildPath("Phases"));
  const std::vector<int32> originalPhases(phases.begin(), phases.end());
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(featuresPath.createChildPath("Centroids")));
  const auto& centroids = dataStructure.getDataRefAs<Float32Array>(featuresPath.createChildPath("Centroids"));
  const std::vector<float32> originalCentroids(centroids.begin(), centroids.end());
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(featuresPath.createChildPath("Biased")));
  const auto& biased = dataStructure.getDataRefAs<BoolArray>(featuresPath.createChildPath("Biased"));
  const std::vector<bool> originalBiased(biased.begin(), biased.end());
  IFilter::ExecuteResult result;
  REQUIRE_NOTHROW(result = ComputeFeatureClusteringFilter{}.execute(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == -54099);
  CHECK(result.result.errors().front().message == fmt::format("Number of Bins for RDF must be at least 1, but was {}.", numberOfBins));
  CHECK(dataStructure.getAllDataPaths() == originalPaths);
  CHECK(std::vector<int32>(phases.begin(), phases.end()) == originalPhases);
  CHECK(std::vector<float32>(centroids.begin(), centroids.end()) == originalCentroids);
  CHECK(std::vector<bool>(biased.begin(), biased.end()) == originalBiased);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: RDF cache indices", "[SimplnxCore][ComputeFeatureClustering]")
{
  CHECK(ComputeFeatureClustering::RdfCacheIndex(2147483647, 3, 5) == uint64{6442450946});
  CHECK(ComputeFeatureClustering::RdfCacheIndex(1048576, 1048576, 7) == uint64{1099511627783});
  CHECK(ComputeFeatureClustering::RdfCacheIndex(4, 2, 3) == 11);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: positive bin count domain", "[SimplnxCore][ComputeFeatureClustering]")
{
  const int32 numberOfBins = GENERATE(1, std::numeric_limits<int32>::max());
  DataStructure dataStructure;
  const Arguments args = CreateBinCountClusteringFixture(dataStructure, numberOfBins);
  const auto preflight = ComputeFeatureClusteringFilter{}.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
