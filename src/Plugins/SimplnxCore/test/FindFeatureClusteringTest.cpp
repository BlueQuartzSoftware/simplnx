#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/FindFeatureClusteringFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_EquivalentDiameters = "EquivalentDiameters";
const std::string k_ExemplarClusteringList = "ClusteringList";
const std::string k_ExemplarRDF = "RDF";
const std::string k_ExemplarMinMaxDistances = "RDFMaxMinDistances";
const std::string k_GeneratedClusteringList = "NX_ClusteringList";
const std::string k_GeneratedRDF = "NX_RDF";
const std::string k_GeneratedMinMaxDistances = "NX_RDFMaxMinDistances";
} // namespace

TEST_CASE("SimplnxCore::FindFeatureClusteringFilter: Valid Filter Execution", "[SimplnxCore][FindFeatureClusteringFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_clustering.tar.gz",
                                                              "6_6_find_feature_clustering.dream3d");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_feature_clustering.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureClusteringFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(FindFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(10));
  args.insertOrAssign(FindFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(2));
  args.insertOrAssign(FindFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(FindFeatureClusteringFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_EquivalentDiameters)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Centroids)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(FindFeatureClusteringFilter::k_ClusteringListArrayName_Key, std::make_any<std::string>(k_GeneratedClusteringList));
  args.insertOrAssign(FindFeatureClusteringFilter::k_RDFArrayName_Key, std::make_any<std::string>(k_GeneratedRDF));
  args.insertOrAssign(FindFeatureClusteringFilter::k_MaxMinArrayName_Key, std::make_any<std::string>(k_GeneratedMinMaxDistances));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareNeighborLists<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_ExemplarClusteringList), k_CellFeatureDataPath.createChildPath(k_GeneratedClusteringList));
  CompareArrays<float32>(dataStructure, k_CellEnsembleAttributeMatrixPath.createChildPath(k_ExemplarRDF), k_CellEnsembleAttributeMatrixPath.createChildPath(k_GeneratedRDF));
  CompareArrays<float32>(dataStructure, k_CellEnsembleAttributeMatrixPath.createChildPath(k_ExemplarMinMaxDistances), k_CellEnsembleAttributeMatrixPath.createChildPath(k_GeneratedMinMaxDistances));
}

TEST_CASE("SimplnxCore::FindFeatureClusteringFilter: InValid Filter Execution", "[SimplnxCore][FindFeatureClusteringFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_clustering.tar.gz",
                                                              "6_6_find_feature_clustering.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_feature_clustering.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureClusteringFilter filter;
  Arguments args;

  // Mismatching cell feature data tuples case :
  args.insertOrAssign(FindFeatureClusteringFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(FindFeatureClusteringFilter::k_NumberOfBins_Key, std::make_any<int32>(10));
  args.insertOrAssign(FindFeatureClusteringFilter::k_PhaseNumber_Key, std::make_any<int32>(2));
  args.insertOrAssign(FindFeatureClusteringFilter::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindFeatureClusteringFilter::k_SetRandomSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindFeatureClusteringFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(FindFeatureClusteringFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_EquivalentDiameters)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_DataContainerPath.createChildPath(k_Cell_Data).createChildPath(k_Phases)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Centroids)));
  args.insertOrAssign(FindFeatureClusteringFilter::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindFeatureClusteringFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(FindFeatureClusteringFilter::k_ClusteringListArrayName_Key, std::make_any<std::string>(k_GeneratedClusteringList));
  args.insertOrAssign(FindFeatureClusteringFilter::k_RDFArrayName_Key, std::make_any<std::string>(k_GeneratedRDF));
  args.insertOrAssign(FindFeatureClusteringFilter::k_MaxMinArrayName_Key, std::make_any<std::string>(k_GeneratedMinMaxDistances));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
