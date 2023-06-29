#include "ComplexCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

TEST_CASE("ComplexCore::ScalarSegmentFeatures", "[Reconstruction][ScalarSegmentFeatures]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_test_data_1.tar.gz", "6_5_test_data_1");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1/6_5_test_data_1.dream3d", complex::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    Arguments args;
    ScalarSegmentFeaturesFilter filter;

    DataPath smallIn100Group({k_DataContainer});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_CellData);
    DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(k_FeatureIds);
    std::string outputFeatureIdsName = "Output_Feature_Ids";
    std::string computedCellDataName = "Computed_CellData";
    DataPath outputFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
    DataPath featureDataGroupPath = smallIn100Group.createChildPath(computedCellDataName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_DataContainer});
    int scalarTolerance = 0;

    // Create default Parameters for the filter.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    // Turn off the use of a Mask Array
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseGoodVoxelsKey, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GoodVoxelsPath_Key, std::make_any<DataPath>(DataPath{}));
    // Set the input array and the tolerance
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    // Set the paths to the created arrays
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsPathKey, std::make_any<std::string>(outputFeatureIdsName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeaturePathKey, std::make_any<std::string>(computedCellDataName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayPathKey, std::make_any<std::string>(k_ActiveName));
    // Are we going to randomize the featureIds when completed.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(activeArrayDataPath);
    size_t numFeatures = actives.getNumberOfTuples();
    std::cout << "NumFeatures: " << numFeatures << std::endl;
    REQUIRE(numFeatures == 847);
  }

  {
    // Write out the DataStructure for later viewing/debugging
    std::string filePath = fmt::format("{}/ScalarSegmentFeatures.dream3d", unit_test::k_BinaryTestOutputDir);
    // std::cout << "Writing file to: " << filePath << std::endl;
    Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(filePath);
    complex::HDF5::FileWriter fileWriter = std::move(result.value());

    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    COMPLEX_RESULT_REQUIRE_VALID(resultH5);
  }
}
