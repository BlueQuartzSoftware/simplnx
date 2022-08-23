#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CalculateFeatureSizesFilter.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{

const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
const std::string k_NumElements("NumElements");

} // namespace

TEST_CASE("ComplexCore::CalculateFeatureSizes", "[ComplexCore][CalculateFeatureSizes]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_stats_test.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(complex::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(complex::Constants::k_FeatureIds);

  std::vector<std::string> featureNames = {k_Volumes, k_EquivalentDiameters, k_NumElements};

  {
    CalculateFeatureSizesFilter filter;
    Arguments args;

    args.insert(CalculateFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insert(CalculateFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(CalculateFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(CalculateFeatureSizesFilter::k_VolumesPath_Key, std::make_any<DataPath>({k_Volumes}));
    args.insert(CalculateFeatureSizesFilter::k_EquivalentDiametersPath_Key, std::make_any<DataPath>({k_EquivalentDiameters}));
    args.insert(CalculateFeatureSizesFilter::k_NumElementsPath_Key, std::make_any<DataPath>({k_NumElements}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare Outputs
  DataPath featureGroup = smallIn100Group.createChildPath(complex::Constants::k_CellFeatureData);
  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_Volumes);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, DataPath({k_Volumes}));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_EquivalentDiameters);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, DataPath({k_EquivalentDiameters}));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_NumElements);
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, DataPath({k_NumElements}));
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
}
