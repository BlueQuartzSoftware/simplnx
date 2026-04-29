#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 * Identify Sample
 * Align Sections Feature Centroid
 *
 * Read DREAM3D File (read the exemplar 'align_sections_feature_centroid.dream3d' file from
 * [Optional] Write out dream3d file
 *
 *
 * Compare the shifts file 'align_sections_feature_centroid.txt' to what was written
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Small IN100 Pipeline", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  UnitTest::LoadPlugins();
  // 1 Z-slice of quats (largest array): 189*201*4*4 = 607824 bytes
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 600000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "neighbor_orientation_correlation.tar.gz", "neighbor_orientation_correlation.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/neighbor_orientation_correlation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteAlignSectionsMisorientation(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir)));

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Align Sections Feature Centroid Filter
  SmallIn100::ExecuteAlignSectionsFeatureCentroid(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir)));

  // Bad Data Neighbor Orientation Check Filter
  SmallIn100::ExecuteBadDataNeighborOrientationCheck(dataStructure, *filterList);

  // Neighbor Orientation Correlation Filter
  {
    auto filter = filterList->createFilter(k_NeighborOrientationCorrelationFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(k_ConfidenceIndexArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>());

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_CellAttributeMatrix));
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cellArrayPath));
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      // Check to see if there is something to compare against in the exemplar file.
      if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
      {
        continue;
      }

      REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath));
      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/neighbor_orientation_correlation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

namespace
{
const std::string k_GeomName("Image Geometry");
const std::string k_CellDataName("Cell Data");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_CIPath = k_CellDataPath.createChildPath("Confidence Index");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_CrystalStructuresPath = k_GeomPath.createChildPath("Ensemble Data").createChildPath("CrystalStructures");

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto quatsDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_QuatsPath, cellTupleShape, {4}, IDataAction::Mode::Execute);
  auto* quatsArray = DataArray<float32>::Create(dataStructure, "Quats", quatsDataStore, cellAM->getId());
  auto& quatsStore = quatsArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_PhasesPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(dataStructure, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  auto ciDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_CIPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* ciArray = DataArray<float32>::Create(dataStructure, "Confidence Index", ciDataStore, cellAM->getId());
  auto& ciStore = ciArray->getDataStoreRef();

  const usize blocksPerDimX = dimX / blockSize;
  const usize blocksPerDimY = dimY / blockSize;

  std::vector<float32> quatsBuf(sliceSize * 4);
  std::vector<int32> phasesBuf(sliceSize);
  std::vector<float32> ciBuf(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        phasesBuf[inSlice] = 1;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        float32 angle = static_cast<float32>(bz * blocksPerDimY * blocksPerDimX + by * blocksPerDimX + bx) * 0.1f;
        float32 sinHalf = std::sin(angle * 0.5f);
        float32 cosHalf = std::cos(angle * 0.5f);

        const usize qIdx = inSlice * 4;
        quatsBuf[qIdx] = cosHalf;
        quatsBuf[qIdx + 1] = sinHalf * 0.577350269f; // 1/sqrt(3)
        quatsBuf[qIdx + 2] = sinHalf * 0.577350269f;
        quatsBuf[qIdx + 3] = sinHalf * 0.577350269f;

        bool isBoundary = (x % blockSize == 0) || (y % blockSize == 0) || (z % blockSize == 0);
        bool isNoisy = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        ciBuf[inSlice] = (isBoundary || isNoisy) ? 0.05f : 0.9f;
      }
    }
    const usize zOffset = z * sliceSize;
    quatsStore.copyFromBuffer(zOffset * 4, nonstd::span<const float32>(quatsBuf.data(), sliceSize * 4));
    phasesStore.copyFromBuffer(zOffset, nonstd::span<const int32>(phasesBuf.data(), sliceSize));
    ciStore.copyFromBuffer(zOffset, nonstd::span<const float32>(ciBuf.data(), sliceSize));
  }

  // Ensemble data — small enough for per-element writes
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Ensemble Data", {2}, imageGeom->getId());
  auto crystalStructuresDataStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, k_CrystalStructuresPath, {2}, {1}, IDataAction::Mode::Execute);
  auto* crystalStructuresArray = DataArray<uint32>::Create(dataStructure, "CrystalStructures", crystalStructuresDataStore, ensembleAM->getId());
  std::array<uint32, 2> csData = {999, 1}; // Unknown, Cubic-High (m-3m)
  crystalStructuresArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(csData.data(), 2));
}
} // namespace

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Generate Test Data", "[OrientationAnalysis][NeighborOrientationCorrelationFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "neighbor_orientation_correlation";
  fs::create_directories(outputDir);

  // Large input data (200x200x200, blockSize=25)
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200, 25);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
    fmt::print("Generated large input: {}\n", (outputDir / "large_input.dream3d").string());
  }
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: 200x200x200 Large OOC", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200x200x200, Quats (float32, 4-comp) => 200*200*4*4 = 640,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 640000, true);

  DYNAMIC_SECTION("forceOoc: " << forceOocAlgo)
  {
    constexpr usize k_Dim = 200;
    constexpr usize k_Block = 25;

    DataStructure dataStructure;
    BuildTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_Block);

    const NeighborOrientationCorrelationFilter filter;
    Arguments args;
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(k_CIPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Some low-CI voxels should have been modified — use Z-slice batched reads for OOC efficiency
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_CIPath));
    const auto& ciAfter = dataStructure.getDataRefAs<Float32Array>(k_CIPath).getDataStoreRef();
    const usize sliceSize = k_Dim * k_Dim;
    std::vector<float32> ciBuf(sliceSize);
    usize modifiedCount = 0;
    for(usize z = 0; z < k_Dim; z++)
    {
      ciAfter.copyIntoBuffer(z * sliceSize, nonstd::span<float32>(ciBuf.data(), sliceSize));
      for(usize y = 0; y < k_Dim; y++)
      {
        for(usize x = 0; x < k_Dim; x++)
        {
          const usize inSlice = y * k_Dim + x;
          bool wasBoundary = (x % k_Block == 0) || (y % k_Block == 0) || (z % k_Block == 0);
          bool wasNoisy = ((x * 7 + y * 13 + z * 29) % 10 == 0);
          if((wasBoundary || wasNoisy) && ciBuf[inSlice] != 0.05f)
          {
            modifiedCount++;
          }
        }
      }
    }
    REQUIRE(modifiedCount > 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
