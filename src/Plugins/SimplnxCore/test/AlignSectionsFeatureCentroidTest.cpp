#include "SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: Algorithm Test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const DataPath k_ExemplarShiftsPath = Constants::k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 600000, true);

  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Align Sections Feature Centroid Filter
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: output test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const std::string k_CentroidsName = "Centroids";

  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 600000, true);

  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Read Exemplar DREAM3D File Filter
  auto baselineFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baselineFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Align Sections Feature Centroid Filter
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentAMName_Key, std::make_any<std::string>(Constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SlicesArrayName_Key, std::make_any<std::string>(Constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_CumulativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Read Exemplar data structure
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/output_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath alignmentAMPath = Constants::k_DataContainerPath.createChildPath(Constants::k_AlignmentAMName);

  const DataPath slicesPath = alignmentAMPath.createChildPath(Constants::k_SlicesArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(slicesPath));
  UnitTest::CompareDataArrays<uint32>(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath), dataStructure.getDataRefAs<IDataArray>(slicesPath));

  const DataPath relativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));

  const DataPath cumulativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));

  const DataPath centroidsPath = alignmentAMPath.createChildPath(k_CentroidsName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(centroidsPath));
  UnitTest::CompareDataArrays<float32>(exemplarDataStructure.getDataRefAs<IDataArray>(centroidsPath), dataStructure.getDataRefAs<IDataArray>(centroidsPath));

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroid: Benchmark 200x200x200", "[SimplnxCore][AlignSectionsFeatureCentroidFilter][.Benchmark]")
{
  UnitTest::LoadPlugins();
  // 200x200x200, largest cell array is EulerAngles float32 3-comp => 200*200*3*4 = 480,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 480000, true);
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kSliceVoxels = kDimX * kDimY;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/align_sections_feature_centroid_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "DataContainer");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "CellData", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    auto* maskArray = UnitTest::CreateTestDataArray<uint8>(buildDS, "Mask", cellTupleShape, {1}, cellAM->getId());
    auto& maskStore = maskArray->getDataStoreRef();
    auto* eulerArray = UnitTest::CreateTestDataArray<float32>(buildDS, "EulerAngles", cellTupleShape, {3}, cellAM->getId());
    auto& eulerStore = eulerArray->getDataStoreRef();
    auto* featureIdsArray = UnitTest::CreateTestDataArray<int32>(buildDS, "FeatureIds", cellTupleShape, {1}, cellAM->getId());
    auto& featureIdsStore = featureIdsArray->getDataStoreRef();

    // Fill using slice-at-a-time bulk writes
    const float32 cx = kDimX / 2.0f;
    const float32 cy = kDimY / 2.0f;
    const float32 radius = 80.0f;

    std::vector<uint8> maskBuf(kSliceVoxels);
    std::vector<float32> eulerBuf(kSliceVoxels * 3);
    std::vector<int32> featureIdsBuf(kSliceVoxels);

    for(usize z = 0; z < kDimZ; z++)
    {
      // Shift center per-slice to create meaningful alignment work
      float32 wobbleX = 10.0f * std::sin(static_cast<float32>(z) * 0.1f);
      float32 wobbleY = 8.0f * std::cos(static_cast<float32>(z) * 0.07f);
      float32 sliceCx = cx + wobbleX;
      float32 sliceCy = cy + wobbleY;

      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize localIdx = y * kDimX + x;
          const float32 dx = static_cast<float32>(x) - sliceCx;
          const float32 dy = static_cast<float32>(y) - sliceCy;
          const float32 dist = std::sqrt(dx * dx + dy * dy);
          maskBuf[localIdx] = dist < radius ? 1 : 0;

          eulerBuf[localIdx * 3 + 0] = static_cast<float32>(x) * 0.01f;
          eulerBuf[localIdx * 3 + 1] = static_cast<float32>(y) * 0.01f;
          eulerBuf[localIdx * 3 + 2] = static_cast<float32>(z) * 0.01f;
          featureIdsBuf[localIdx] = static_cast<int32>(maskBuf[localIdx]);
        }
      }
      maskStore.copyFromBuffer(z * kSliceVoxels, nonstd::span<const uint8>(maskBuf.data(), kSliceVoxels));
      eulerStore.copyFromBuffer(z * kSliceVoxels * 3, nonstd::span<const float32>(eulerBuf.data(), kSliceVoxels * 3));
      featureIdsStore.copyFromBuffer(z * kSliceVoxels, nonstd::span<const int32>(featureIdsBuf.data(), kSliceVoxels));
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become OOC-backed) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    AlignSectionsFeatureCentroidFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Mask"})));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  fs::remove(benchmarkFile);
}
