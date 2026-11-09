#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <limits>

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
  CHECK(result.outputActions.errors().front().code == ComputeFeatureClustering::k_InvalidNumberOfBins);
  CHECK(result.outputActions.errors().front().message == "Number of Bins for RDF must be at least 1, but was -1.");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureClustering: direct bin count guard", "[SimplnxCore][ComputeFeatureClustering]")
{
  const int32 numberOfBins = GENERATE(0, -1);
  CAPTURE(numberOfBins);
  DataStructure dataStructure;
  ComputeFeatureClusteringInputValues inputValues{};
  inputValues.NumberOfBins = numberOfBins;
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{};
  ComputeFeatureClustering algorithm(dataStructure, messageHandler, shouldCancel, &inputValues);
  Result<> result;
  REQUIRE_NOTHROW(result = algorithm());
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == ComputeFeatureClustering::k_InvalidNumberOfBins);
  CHECK(result.errors().front().message == fmt::format("Number of Bins for RDF must be at least 1, but was {}.", numberOfBins));
  CHECK(dataStructure.getAllDataPaths().empty());
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
  const auto result = ComputeFeatureClustering::ValidateNumberOfBins(numberOfBins);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  DataStructure dataStructure;
  const Arguments args = CreateBinCountClusteringFixture(dataStructure, numberOfBins);
  const auto preflight = ComputeFeatureClusteringFilter{}.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
