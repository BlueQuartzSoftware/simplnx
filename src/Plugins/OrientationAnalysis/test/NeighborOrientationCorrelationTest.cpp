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
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 600000, true);

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
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Benchmark 200x200x200", "[OrientationAnalysis][NeighborOrientationCorrelationFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  // 200x200x200, Quats (float32, 4-comp) => 200*200*4*4 = 640,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 640000, true);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/neighbor_orientation_correlation_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "Image Geometry");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "Cell Data", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    // Quaternions: block-based orientations with noise at boundaries
    auto* quatsArray = UnitTest::CreateTestDataArray<float32>(buildDS, "Quats", cellTupleShape, {4}, cellAM->getId());
    auto& quatsStore = quatsArray->getDataStoreRef();

    // Phases: all phase 1 (cubic)
    auto* phasesArray = UnitTest::CreateTestDataArray<int32>(buildDS, "Phases", cellTupleShape, {1}, cellAM->getId());
    auto& phasesStore = phasesArray->getDataStoreRef();

    // Confidence Index: low at block boundaries, high inside blocks
    auto* ciArray = UnitTest::CreateTestDataArray<float32>(buildDS, "Confidence Index", cellTupleShape, {1}, cellAM->getId());
    auto& ciStore = ciArray->getDataStoreRef();

    constexpr usize kBlockSize = 25;
    constexpr usize kBlocksPerDim = kDimX / kBlockSize;
    for(usize z = 0; z < kDimZ; z++)
    {
      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize idx = z * kDimX * kDimY + y * kDimX + x;
          phasesStore[idx] = 1;

          // Block-based orientation: each 25^3 block gets a distinct quaternion
          usize bx = x / kBlockSize;
          usize by = y / kBlockSize;
          usize bz = z / kBlockSize;
          float32 angle = static_cast<float32>(bz * kBlocksPerDim * kBlocksPerDim + by * kBlocksPerDim + bx) * 0.1f;
          float32 sinHalf = std::sin(angle * 0.5f);
          float32 cosHalf = std::cos(angle * 0.5f);

          const usize qIdx = idx * 4;
          quatsStore[qIdx] = cosHalf;
          quatsStore[qIdx + 1] = sinHalf * 0.577350269f; // 1/sqrt(3)
          quatsStore[qIdx + 2] = sinHalf * 0.577350269f;
          quatsStore[qIdx + 3] = sinHalf * 0.577350269f;

          // Low CI at block boundaries (~10% of voxels), high CI inside
          bool isBoundary = (x % kBlockSize == 0) || (y % kBlockSize == 0) || (z % kBlockSize == 0);
          bool isNoisy = ((x * 7 + y * 13 + z * 29) % 10 == 0);
          ciStore[idx] = (isBoundary || isNoisy) ? 0.05f : 0.9f;
        }
      }
    }

    // Ensemble data: Crystal Structures (1 entry for phase 0 = unknown, 1 entry for phase 1 = cubic)
    auto* ensembleAM = AttributeMatrix::Create(buildDS, "Ensemble Data", {2}, imageGeom->getId());
    auto* crystalStructuresArray = UnitTest::CreateTestDataArray<uint32>(buildDS, "CrystalStructures", {2}, {1}, ensembleAM->getId());
    auto& crystalStructuresStore = crystalStructuresArray->getDataStoreRef();
    crystalStructuresStore[0] = 999; // Unknown
    crystalStructuresStore[1] = 1;   // Cubic-High (m-3m)

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become ZarrStore in OOC) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    const NeighborOrientationCorrelationFilter filter;
    Arguments args;

    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "Cell Data", "Confidence Index"})));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "Cell Data", "Phases"})));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "Cell Data", "Quats"})));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "Ensemble Data", "CrystalStructures"})));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
