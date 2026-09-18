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

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the exemplar before the selected algorithm scenario.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Execute the alignment with a fixed reference slice.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter: output test", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  const std::string k_CentroidsName = "Centroids";

  UnitTest::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the in-core and OOC scenarios for this test.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_feature_centroids.tar.gz", "align_sections_feature_centroids");

  // Load the baseline before the selected algorithm scenario.
  auto baselineFilePath = fs::path(fmt::format("{}/align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baselineFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath));

  // Execute the alignment and retain its diagnostic shift arrays.
  {
    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
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

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Load the expected shift arrays from the output exemplar.
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

// The optional output supports manual inspection of the aligned result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
