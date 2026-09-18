#include "OrientationAnalysis/Filters/ComputeGBCDFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <span>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
constexpr StringLiteral k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

constexpr usize k_GbcdChunkBoundaryFaceCount = 100003;
// These placements cross two full 50,000-face chunks and the partial final chunk.
constexpr std::array<usize, 6> k_GbcdContributorFaceIndices = {0, 49999, 50000, 99999, 100000, 100002};
constexpr std::array<std::array<float64, 3>, 6> k_GbcdContributorNormals = {
    std::array<float64, 3>{3.0, 2.0, 1.0}, std::array<float64, 3>{3.0, 1.0, 2.0}, std::array<float64, 3>{2.0, 3.0, 1.0},
    std::array<float64, 3>{1.0, 3.0, 2.0}, std::array<float64, 3>{2.0, 1.0, 3.0}, std::array<float64, 3>{1.0, 2.0, 3.0},
};
constexpr std::array<float64, 6> k_GbcdContributorAreas = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0};

struct GbcdBoundaryFixture
{
  DataStructure dataStructure;
  DataPath triangleGeometryPath;
  DataPath faceLabelsPath;
  DataPath faceNormalsPath;
  DataPath faceAreasPath;
  DataPath featureEulerAnglesPath;
  DataPath featurePhasesPath;
  DataPath crystalStructuresPath;
  DataPath outputPath;
};

/**
 * @brief Creates an in-core GBCD fixture with selected contributing faces.
 * @param faceCount Specifies the number of face tuples.
 * @param contributorFaceIndices Identifies the face tuple for each contributor.
 * @param contributorIndices Identifies the normal and area for each contributor.
 * @return Fixture with zero values and negative labels for skipped faces.
 */
GbcdBoundaryFixture CreateGbcdBoundaryFixture(const usize faceCount, const std::span<const usize> contributorFaceIndices, const std::span<const usize> contributorIndices)
{
  REQUIRE(contributorFaceIndices.size() == contributorIndices.size());

  GbcdBoundaryFixture fixture;
  fixture.triangleGeometryPath = DataPath({"GBCD Triangle Geometry"});
  const DataPath faceDataPath = fixture.triangleGeometryPath.createChildPath(Constants::k_FaceData);
  const DataPath featureDataPath = fixture.triangleGeometryPath.createChildPath(Constants::k_Grain_Data);
  const DataPath phaseDataPath = fixture.triangleGeometryPath.createChildPath(Constants::k_Phase_Data);
  fixture.faceLabelsPath = faceDataPath.createChildPath(Constants::k_FaceLabels);
  fixture.faceNormalsPath = faceDataPath.createChildPath(Constants::k_FaceNormals);
  fixture.faceAreasPath = faceDataPath.createChildPath(Constants::k_FaceAreas);
  fixture.featureEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  fixture.featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);
  fixture.crystalStructuresPath = phaseDataPath.createChildPath(Constants::k_CrystalStructures);
  fixture.outputPath = fixture.triangleGeometryPath.createChildPath("GBCD Face Ensemble Data").createChildPath(Constants::k_GBCD_Name);

  auto* triangleGeomPtr = TriangleGeom::Create(fixture.dataStructure, fixture.triangleGeometryPath.getTargetName());
  REQUIRE(triangleGeomPtr != nullptr);
  auto* faceDataPtr = AttributeMatrix::Create(fixture.dataStructure, faceDataPath.getTargetName(), {faceCount}, triangleGeomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(fixture.dataStructure, featureDataPath.getTargetName(), {3}, triangleGeomPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  auto* phaseDataPtr = AttributeMatrix::Create(fixture.dataStructure, phaseDataPath.getTargetName(), {2}, triangleGeomPtr->getId());
  REQUIRE(phaseDataPtr != nullptr);

  auto faceLabelsStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.faceLabelsPath, {faceCount}, {2});
  auto* faceLabelsArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.faceLabelsPath.getTargetName(), faceLabelsStore, faceDataPtr->getId());
  REQUIRE(faceLabelsArrayPtr != nullptr);
  faceLabelsStore->fill(-1);

  auto faceNormalsStore = DataStoreUtilities::CreateDataStore<float64>(fixture.dataStructure, fixture.faceNormalsPath, {faceCount}, {3});
  auto* faceNormalsArrayPtr = Float64Array::Create(fixture.dataStructure, fixture.faceNormalsPath.getTargetName(), faceNormalsStore, faceDataPtr->getId());
  REQUIRE(faceNormalsArrayPtr != nullptr);
  faceNormalsStore->fill(0.0);

  auto faceAreasStore = DataStoreUtilities::CreateDataStore<float64>(fixture.dataStructure, fixture.faceAreasPath, {faceCount}, {1});
  auto* faceAreasArrayPtr = Float64Array::Create(fixture.dataStructure, fixture.faceAreasPath.getTargetName(), faceAreasStore, faceDataPtr->getId());
  REQUIRE(faceAreasArrayPtr != nullptr);
  faceAreasStore->fill(0.0);

  auto eulerAnglesStore = DataStoreUtilities::CreateDataStore<float32>(fixture.dataStructure, fixture.featureEulerAnglesPath, {3}, {3});
  auto* eulerAnglesArrayPtr = Float32Array::Create(fixture.dataStructure, fixture.featureEulerAnglesPath.getTargetName(), eulerAnglesStore, featureDataPtr->getId());
  REQUIRE(eulerAnglesArrayPtr != nullptr);
  eulerAnglesStore->fill(0.0F);

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.featurePhasesPath, {3}, {1});
  auto* featurePhasesArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.featurePhasesPath.getTargetName(), featurePhasesStore, featureDataPtr->getId());
  REQUIRE(featurePhasesArrayPtr != nullptr);
  featurePhasesStore->fill(0);
  (*featurePhasesStore)[1] = 1;
  (*featurePhasesStore)[2] = 1;

  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(fixture.dataStructure, fixture.crystalStructuresPath, {2}, {1});
  auto* crystalStructuresArrayPtr = UInt32Array::Create(fixture.dataStructure, fixture.crystalStructuresPath.getTargetName(), crystalStructuresStore, phaseDataPtr->getId());
  REQUIRE(crystalStructuresArrayPtr != nullptr);
  crystalStructuresStore->fill(0);
  (*crystalStructuresStore)[1] = 4;

  const float64 normalization = 1.0 / std::sqrt(14.0);
  for(usize contributorIdx = 0; contributorIdx < contributorIndices.size(); contributorIdx++)
  {
    const usize faceIdx = contributorFaceIndices[contributorIdx];
    const usize sourceIdx = contributorIndices[contributorIdx];
    REQUIRE(faceIdx < faceCount);
    REQUIRE(sourceIdx < k_GbcdContributorNormals.size());
    (*faceLabelsStore)[faceIdx * 2] = 1;
    (*faceLabelsStore)[faceIdx * 2 + 1] = 2;
    for(usize componentIdx = 0; componentIdx < 3; componentIdx++)
    {
      (*faceNormalsStore)[faceIdx * 3 + componentIdx] = k_GbcdContributorNormals[sourceIdx][componentIdx] * normalization;
    }
    (*faceAreasStore)[faceIdx] = k_GbcdContributorAreas[sourceIdx];
  }

  return fixture;
}

void ExecuteGbcd(GbcdBoundaryFixture& fixture)
{
  ComputeGBCDFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDFilter::k_GBCDRes_Key, std::make_any<float32>(9.0F));
  args.insertOrAssign(ComputeGBCDFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(fixture.triangleGeometryPath));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(fixture.faceLabelsPath));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(fixture.faceNormalsPath));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(fixture.faceAreasPath));
  args.insertOrAssign(ComputeGBCDFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(fixture.featureEulerAnglesPath));
  args.insertOrAssign(ComputeGBCDFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(fixture.featurePhasesPath));
  args.insertOrAssign(ComputeGBCDFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(fixture.crystalStructuresPath));
  args.insertOrAssign(ComputeGBCDFilter::k_FaceEnsembleAttributeMatrixName_Key, std::make_any<std::string>("GBCD Face Ensemble Data"));
  args.insertOrAssign(ComputeGBCDFilter::k_GBCDArrayName_Key, std::make_any<std::string>(Constants::k_GBCD_Name));

  const auto preflightResult = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

/**
 * @brief Gets the finite positive GBCD bins.
 * @param gbcdArrayRef Contains the GBCD output.
 * @return Indices of finite positive bins.
 */
std::set<usize> GetPositiveGbcdBins(const Float64Array& gbcdArrayRef)
{
  std::set<usize> populatedBins;
  for(usize binIdx = 0; binIdx < gbcdArrayRef.getSize(); binIdx++)
  {
    const float64 value = gbcdArrayRef[binIdx];
    if(std::isnan(value))
    {
      continue;
    }
    REQUIRE(std::isfinite(value));
    if(value > 0.0)
    {
      populatedBins.insert(binIdx);
    }
  }
  return populatedBins;
}

/**
 * @brief Requires exact GBCD equality with matching NaN classification.
 * @param actualArrayRef Contains the expanded output.
 * @param expectedArrayRef Contains the compact oracle output.
 */
void RequireExactGbcdEquality(const Float64Array& actualArrayRef, const Float64Array& expectedArrayRef)
{
  REQUIRE(actualArrayRef.getSize() == expectedArrayRef.getSize());
  for(usize binIdx = 0; binIdx < actualArrayRef.getSize(); binIdx++)
  {
    const float64 actual = actualArrayRef[binIdx];
    const float64 expected = expectedArrayRef[binIdx];
    // The shared comparison helper cannot reject a one-sided NaN.
    REQUIRE(std::isnan(actual) == std::isnan(expected));
    if(!std::isnan(actual))
    {
      REQUIRE(std::isfinite(actual));
      REQUIRE(std::isfinite(expected));
      REQUIRE(actual == expected);
    }
  }
}

} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBCD", "[OrientationAnalysis][ComputeGBCD]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  DataPath featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);

  DataPath ensembleDataPath = smallIn100Group.createChildPath(Constants::k_Phase_Data);
  DataPath crystalStructurePath = ensembleDataPath.createChildPath(Constants::k_CrystalStructures);

  DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeGBCDFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeGBCDFilter::k_GBCDRes_Key, std::make_any<Float32Parameter::ValueType>(9.0F));

    args.insertOrAssign(ComputeGBCDFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(triangleDataContainerPath));

    args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceLabels));
    args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceNormals));
    args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceAreas));
    args.insertOrAssign(ComputeGBCDFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeGBCDFilter::k_FeaturePhasesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(featurePhasesPath));
    args.insertOrAssign(ComputeGBCDFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDFilter::k_FaceEnsembleAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FaceEnsembleDataPath));
    args.insertOrAssign(ComputeGBCDFilter::k_GBCDArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(Constants::k_GBCD_Name));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output GBCD Data
  {
    const DataPath k_GeneratedDataPath = faceEnsemblePath.createChildPath(Constants::k_GBCD_Name);
    const DataPath k_ExemplarArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(Constants::k_GBCD_Name);

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_gbcd.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDFilter: Chunk Boundaries", "[OrientationAnalysis][ComputeGBCDFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  constexpr std::array<usize, 6> k_CompactContributorFaceIndices = {0, 1, 2, 3, 4, 5};
  constexpr std::array<usize, 6> k_ContributorIndices = {0, 1, 2, 3, 4, 5};

  std::array<std::set<usize>, 6> singleFaceBinSets;
  std::set<usize> singleFaceBinUnion;
  // Per-face support prevents MRD normalization from hiding a dropped contributor.
  for(usize contributorIdx = 0; contributorIdx < k_ContributorIndices.size(); contributorIdx++)
  {
    const std::array<usize, 1> faceIndices = {0};
    const std::array<usize, 1> contributorIndices = {contributorIdx};
    GbcdBoundaryFixture singleFaceFixture = CreateGbcdBoundaryFixture(1, faceIndices, contributorIndices);
    ExecuteGbcd(singleFaceFixture);

    REQUIRE_NOTHROW(singleFaceFixture.dataStructure.getDataRefAs<Float64Array>(singleFaceFixture.outputPath));
    const auto& singleFaceGbcdArrayRef = singleFaceFixture.dataStructure.getDataRefAs<Float64Array>(singleFaceFixture.outputPath);
    singleFaceBinSets[contributorIdx] = GetPositiveGbcdBins(singleFaceGbcdArrayRef);
    REQUIRE(singleFaceBinSets[contributorIdx].size() == 2);
    const float64 expectedPopulatedValue = static_cast<float64>(singleFaceGbcdArrayRef.getNumberOfComponents()) / 2.0;
    for(const usize binIdx : singleFaceBinSets[contributorIdx])
    {
      REQUIRE(singleFaceGbcdArrayRef[binIdx] == expectedPopulatedValue);
    }
    singleFaceBinUnion.insert(singleFaceBinSets[contributorIdx].begin(), singleFaceBinSets[contributorIdx].end());
  }

  bool allSingleFaceBinSetsIdentical = true;
  for(usize contributorIdx = 1; contributorIdx < singleFaceBinSets.size(); contributorIdx++)
  {
    if(singleFaceBinSets[contributorIdx] != singleFaceBinSets.front())
    {
      allSingleFaceBinSetsIdentical = false;
      break;
    }
  }
  REQUIRE_FALSE(allSingleFaceBinSetsIdentical);

  for(usize contributorIdx = 0; contributorIdx < singleFaceBinSets.size(); contributorIdx++)
  {
    bool hasBinExclusiveToContributor = false;
    for(const usize binIdx : singleFaceBinSets[contributorIdx])
    {
      bool presentInAnotherContributor = false;
      for(usize otherContributorIdx = 0; otherContributorIdx < singleFaceBinSets.size(); otherContributorIdx++)
      {
        if(otherContributorIdx != contributorIdx && singleFaceBinSets[otherContributorIdx].contains(binIdx))
        {
          presentInAnotherContributor = true;
          break;
        }
      }
      if(!presentInAnotherContributor)
      {
        hasBinExclusiveToContributor = true;
        break;
      }
    }
    REQUIRE(hasBinExclusiveToContributor);
  }

  GbcdBoundaryFixture compactFixture = CreateGbcdBoundaryFixture(k_CompactContributorFaceIndices.size(), k_CompactContributorFaceIndices, k_ContributorIndices);
  ExecuteGbcd(compactFixture);
  REQUIRE_NOTHROW(compactFixture.dataStructure.getDataRefAs<Float64Array>(compactFixture.outputPath));
  const auto& compactGbcdArrayRef = compactFixture.dataStructure.getDataRefAs<Float64Array>(compactFixture.outputPath);
  const std::set<usize> compactBinSet = GetPositiveGbcdBins(compactGbcdArrayRef);
  REQUIRE(compactBinSet == singleFaceBinUnion);

  GbcdBoundaryFixture expandedFixture = CreateGbcdBoundaryFixture(k_GbcdChunkBoundaryFaceCount, k_GbcdContributorFaceIndices, k_ContributorIndices);
  ExecuteGbcd(expandedFixture);
  REQUIRE_NOTHROW(expandedFixture.dataStructure.getDataRefAs<Float64Array>(expandedFixture.outputPath));
  const auto& expandedGbcdArrayRef = expandedFixture.dataStructure.getDataRefAs<Float64Array>(expandedFixture.outputPath);
  RequireExactGbcdEquality(expandedGbcdArrayRef, compactGbcdArrayRef);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeGBCDFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_Small_IN100_GBCD" / "6_6_Small_IN100_GBCD.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);

  const DataPath featureDataPath = DataPath({Constants::k_SmallIN100}).createChildPath(Constants::k_Grain_Data);
  const DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  const DataPath featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);
  const DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100}).createChildPath(Constants::k_Phase_Data).createChildPath(Constants::k_CrystalStructures);
  const DataPath triangleGeometryPath({Constants::k_TriangleDataContainerName});
  const DataPath faceDataPath = triangleGeometryPath.createChildPath(Constants::k_FaceData);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featurePhasesPath));
  auto& featurePhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath));
  auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath);

  ComputeGBCDFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDFilter::k_GBCDRes_Key, std::make_any<Float32Parameter::ValueType>(9.0F));
  args.insertOrAssign(ComputeGBCDFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeometryPath));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceLabels)));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceNormals)));
  args.insertOrAssign(ComputeGBCDFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceAreas)));
  args.insertOrAssign(ComputeGBCDFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  args.insertOrAssign(ComputeGBCDFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
  args.insertOrAssign(ComputeGBCDFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(ComputeGBCDFilter::k_FaceEnsembleAttributeMatrixName_Key, std::make_any<std::string>("Bounds Face Ensemble Data"));
  args.insertOrAssign(ComputeGBCDFilter::k_GBCDArrayName_Key, std::make_any<std::string>("Bounds GBCD"));

  SECTION("Participating Phase returns an error")
  {
    auto& featurePhasesStoreRef = featurePhasesArrayRef.getDataStoreRef();
    for(usize featureIdx = 1; featureIdx < featurePhasesStoreRef.getNumberOfTuples(); featureIdx++)
    {
      featurePhasesStoreRef[featureIdx] = static_cast<int32>(crystalStructuresArrayRef.getNumberOfTuples());
    }
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -75000);
  }

  SECTION("Participating Laue index returns an error")
  {
    crystalStructuresArrayRef.getDataStoreRef()[1] = 999U;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -75001);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeGBCDFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGBCDFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeGBCDFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeGBCDFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(ComputeGBCDFilter::k_GBCDRes_Key) == 2.5f);
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_SelectedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_SurfaceMeshFaceAreasArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeGBCDFilter::k_FaceEnsembleAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeGBCDFilter::k_GBCDArrayName_Key) == "TestName");
    }
  }
}
