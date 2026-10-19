#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "ComputeSurfaceAreaToVolumeTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SurfaceAreaVolumeRationArrayName("SurfaceAreaVolumeRatio");
const std::string k_SphericityArrayName("Sphericity");
const std::string k_SurfaceAreaVolumeRationArrayNameNX("SurfaceAreaVolumeRatioNX");
const std::string k_SphericityArrayNameNX("SphericityNX");

constexpr usize k_Dim = 200;
constexpr usize k_TotalVoxels = k_Dim * k_Dim * k_Dim;
constexpr int32 k_NumOctantFeatures = 8;
const std::string k_GeomName = "ImageGeom";

void BuildOctantWithNumCells(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_Dim, k_Dim, k_Dim};

  auto* imageGeom = ImageGeom::Create(ds, k_GeomName);
  imageGeom->setDimensions({k_Dim, k_Dim, k_Dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({k_GeomName, Constants::k_CellData, Constants::k_FeatureIds});
  auto fidsStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1});
  auto* featureIds = DataArray<int32>::Create(ds, Constants::k_FeatureIds, fidsStore, cellAM->getId());
  auto& fidsRef = featureIds->getDataStoreRef();

  std::vector<int32> featureCounts(k_NumOctantFeatures + 1, 0);
  const usize sliceSize = k_Dim * k_Dim;
  std::vector<int32> sliceBuffer(sliceSize);
  for(usize iz = 0; iz < k_Dim; iz++)
  {
    for(usize iy = 0; iy < k_Dim; iy++)
    {
      for(usize ix = 0; ix < k_Dim; ix++)
      {
        const usize inSlice = iy * k_Dim + ix;
        const usize globalIdx = iz * sliceSize + inSlice;
        const int32 octant = static_cast<int32>((iz >= k_Dim / 2 ? 4 : 0) + (iy >= k_Dim / 2 ? 2 : 0) + (ix >= k_Dim / 2 ? 1 : 0)) + 1;
        const int32 fid = ((globalIdx * 7 + 13) % 100) < 5 ? 0 : octant;
        sliceBuffer[inSlice] = fid;
        if(fid > 0)
        {
          featureCounts[fid]++;
        }
      }
    }
    auto fidsRefWriteResult = fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(fidsRefWriteResult);
  }

  auto* cellFeatureAM = AttributeMatrix::Create(ds, Constants::k_CellFeatureData, {static_cast<usize>(k_NumOctantFeatures + 1)}, imageGeom->getId());

  // NumCells is small (9 tuples) — per-element writes are fine
  const DataPath numCellsPath = DataPath({k_GeomName, Constants::k_CellFeatureData, Constants::k_NumElements});
  auto numCellsStore = DataStoreUtilities::CreateDataStore<int32>(ds, numCellsPath, {static_cast<usize>(k_NumOctantFeatures + 1)}, {1});
  auto* numCells = DataArray<int32>::Create(ds, Constants::k_NumElements, numCellsStore, cellFeatureAM->getId());
  auto& numCellsRef = numCells->getDataStoreRef();
  std::vector<int32> localNumCells(featureCounts.begin(), featureCounts.end());
  auto numCellsRefWriteResult = numCellsRef.copyFromBuffer(0, nonstd::span<const int32>(localNumCells.data(), localNumCells.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(numCellsRefWriteResult);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: geometry and FeatureIds tuple cardinality", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto labelDimensions = GENERATE(SizeVec3{2, 2, 2}, SizeVec3{6, 5, 4}, SizeVec3{6, 5, 2});
  const auto calculateSphericity = GENERATE(false, true);
  const ShapeType labelShape = {labelDimensions[2], labelDimensions[1], labelDimensions[0]};
  const usize labelCount = labelDimensions[0] * labelDimensions[1] * labelDimensions[2];
  CAPTURE(labelCount, calculateSphericity);

  DataStructure dataStructure;
  const DataPath geomPath({"SelectedGeometry"});
  const DataPath labelGeomPath({"LabelGeometry"});
  const DataPath idsPath = labelGeomPath.createChildPath("CellData").createChildPath("FeatureIds");
  const DataPath countsPath = geomPath.createChildPath("CellFeatureData").createChildPath("NumCells");
  const DataPath ratioPath = countsPath.replaceName("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = countsPath.replaceName("Sphericity");

  auto* geom = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  REQUIRE(geom != nullptr);
  geom->setDimensions({4, 3, 5});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {5, 3, 4}, geom->getId());
  REQUIRE(cellAM != nullptr);
  geom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "CellFeatureData", {2}, geom->getId());
  REQUIRE(featureAM != nullptr);

  auto* labelGeom = ImageGeom::Create(dataStructure, labelGeomPath.getTargetName());
  REQUIRE(labelGeom != nullptr);
  labelGeom->setDimensions(labelDimensions);
  auto* labelAM = AttributeMatrix::Create(dataStructure, "CellData", labelShape, labelGeom->getId());
  REQUIRE(labelAM != nullptr);
  labelGeom->setCellData(*labelAM);

  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, labelShape, {1});
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, {2}, {1});
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStore, labelAM->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStore, featureAM->getId()) != nullptr);

  ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(countsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));

  auto preflight = filter.preflight(dataStructure, args);
  if(labelCount == 60)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    CHECK(preflight.outputActions.value().actions.size() == (calculateSphericity ? 2 : 1));
  }
  else
  {
    // An invalid output-actions result exposes no actions that could allocate outputs for the unsafe input.
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    const auto& error = preflight.outputActions.errors().front();
    CHECK(error.code == -12804);
    CHECK(error.message.find(idsPath.toString()) != std::string::npos);
    CHECK(error.message.find(geomPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("{} tuples", labelCount)) != std::string::npos);
    CHECK(error.message.find("60 cells") != std::string::npos);
  }
  CHECK(dataStructure.getDataAs<IDataArray>(ratioPath) == nullptr);
  CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: rejects non-DataArray selections", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const bool replaceFeatureIds = GENERATE(true, false);
  const bool useStringArray = GENERATE(true, false);
  CAPTURE(replaceFeatureIds, useStringArray);

  DataStructure dataStructure;
  const DataPath geomPath({"ImageGeom"});
  const DataPath cellPath = geomPath.createChildPath("CellData");
  const DataPath featurePath = geomPath.createChildPath("CellFeatureData");
  const DataPath idsPath = cellPath.createChildPath("FeatureIds");
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const DataPath ratioPath = featurePath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = featurePath.createChildPath("Sphericity");
  const ShapeType cellShape = {2, 2, 2};
  const ShapeType featureShape = {2};
  const std::vector<int32> labels(8, 1);
  const std::vector<int32> counts = {0, 8};

  auto* geom = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  REQUIRE(geom != nullptr);
  geom->setDimensions({2, 2, 2});
  auto* cellAM = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), cellShape, geom->getId());
  REQUIRE(cellAM != nullptr);
  geom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, featurePath.getTargetName(), featureShape, geom->getId());
  REQUIRE(featureAM != nullptr);
  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, {1});
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, {1});
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, idsPath.getTargetName(), idsStore, cellAM->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, countsPath.getTargetName(), countsStore, featureAM->getId()) != nullptr);
  const auto idsWrite = idsStore->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  const auto countsWrite = countsStore->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  const auto* parent = replaceFeatureIds ? cellAM : featureAM;
  const DataPath parentPath = replaceFeatureIds ? cellPath : featurePath;
  const auto& values = replaceFeatureIds ? labels : counts;
  const std::string malformedName = useStringArray ? "StringSelection" : "NeighborListSelection";
  const DataPath malformedPath = parentPath.createChildPath(malformedName);
  REQUIRE(values.size() == parent->getNumberOfTuples());
  if(useStringArray)
  {
    std::vector<std::string> strings;
    strings.reserve(values.size());
    for(const int32 value : values)
    {
      strings.push_back(std::to_string(value));
    }
    REQUIRE(StringArray::CreateWithValues(dataStructure, malformedName, parent->getShape(), std::move(strings), parent->getId()) != nullptr);
  }
  else
  {
    auto* lists = NeighborList<int32>::Create(dataStructure, malformedName, parent->getShape(), parent->getId());
    REQUIRE(lists != nullptr);
    for(usize index = 0; index < values.size(); ++index)
    {
      lists->setList(static_cast<int32>(index), std::vector<int32>{values[index]});
    }
    CHECK(lists->getComponentShape() == ShapeType{1});
  }

  ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(replaceFeatureIds ? malformedPath : idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(replaceFeatureIds ? countsPath : malformedPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));

  IFilter::PreflightResult preflight;
  REQUIRE_NOTHROW(preflight = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.errors().size() == 1);
  const auto& error = preflight.outputActions.errors().front();
  CHECK(error.code == (replaceFeatureIds ? -12803 : -12801));
  CHECK(error.message.find(malformedPath.toString()) != std::string::npos);
  if(replaceFeatureIds)
  {
    CHECK(error.message.find(useStringArray ? "StringArray" : "NeighborList<int32>") != std::string::npos);
    CHECK(error.message.find("scalar int32 DataArray") != std::string::npos);
    CHECK(error.message.find("Select a scalar int32 DataArray") != std::string::npos);
  }
  CHECK(dataStructure.getData(ratioPath) == nullptr);
  CHECK(dataStructure.getData(sphericityPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical interior pair", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto pairAxis = GENERATE(usize{0}, usize{1}, usize{2});
  const auto spacing = GENERATE(FloatVec3{2.0f, 3.0f, 5.0f}, FloatVec3{1.0f, 1.0f, 1.0f});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  SurfaceAreaToVolumeTest::CheckInteriorPairSurfaceMetrics(scope, pairAxis, spacing, calculateSphericity);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical single-slice pair", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto pairAxis = GENERATE(usize{0}, usize{1});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  SurfaceAreaToVolumeTest::CheckInteriorPairSurfaceMetrics(scope, pairAxis, FloatVec3{2.0f, 3.0f, 5.0f}, calculateSphericity, true);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: decimal connected comb", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto spacing = GENERATE(FloatVec3{0.13f, 0.27f, 1.1f}, FloatVec3{0.13f, 0.13f, 0.13f});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  SurfaceAreaToVolumeTest::CheckConnectedCombSurfaceMetrics(scope, spacing, calculateSphericity);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: scale-invariant sphericity", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  SurfaceAreaToVolumeTest::CheckScaleInvariantSphericity(scope);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: nonpositive supplied pair counts", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto suppliedPairCount = GENERATE(int32{0}, int32{-2});
  CAPTURE(scenario, suppliedPairCount);
  UnitTest::AlgorithmTestScope scope(scenario);
  SurfaceAreaToVolumeTest::CheckInteriorPairSurfaceMetrics(scope, 0, FloatVec3{2.0f, 3.0f, 5.0f}, true, false, suppliedPairCount);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolume", "[SimplnxCore][ComputeSurfaceAreaToVolume]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Configure the filter arguments.
  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_NumElementsArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_NumElementsArrayPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_SurfaceAreaVolumeRationArrayNameNX));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_SphericityArrayNameNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayName});
    DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    exemplarPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayName});
    calculatedPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_surface_area_volume_ratio.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Generate Large Test Dataset", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildOctantWithNumCells(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "compute_surface_area_to_volume_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: 200x200x200 octant features", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildOctantWithNumCells(dataStructure);

  const DataPath benchGeomPath({k_GeomName});
  const DataPath benchFeatureIdsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  const DataPath benchCellFeatureAMPath = benchGeomPath.createChildPath(Constants::k_CellFeatureData);
  const DataPath benchNumCellsPath = benchCellFeatureAMPath.createChildPath(Constants::k_NumElements);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));

  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(benchGeomPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(benchFeatureIdsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(benchNumCellsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: SA/V ratios and sphericity should be positive for all features
  const DataPath savPath = benchCellFeatureAMPath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = benchCellFeatureAMPath.createChildPath("Sphericity");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(savPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath));
  const auto& savRatio = dataStructure.getDataRefAs<DataArray<float32>>(savPath).getDataStoreRef();
  const auto& sphericity = dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath).getDataStoreRef();
  for(int32 f = 1; f <= k_NumOctantFeatures; f++)
  {
    REQUIRE(savRatio[f] > 0.0f);
    REQUIRE(sphericity[f] > 0.0f);
    REQUIRE(sphericity[f] <= 1.0f);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
