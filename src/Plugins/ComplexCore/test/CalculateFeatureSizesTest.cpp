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

inline DataStructure LoadDataStructure(const fs::path& filepath)
{
  DataStructure exemplarDataStructure;
  REQUIRE(fs::exists(filepath));
  auto result = DREAM3D::ImportDataStructureFromFile(filepath);
  REQUIRE(result.valid());
  return result.value();
}
} // namespace

TEST_CASE("ComplexCore::CalculateFeatureSizes", "[ComplexCore][CalculateFeatureSizes]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1.dream3d", unit_test::k_TestDataSourceDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);
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

  // Output
  DataPath featureGroup = smallIn100Group.createChildPath(complex::Constants::k_CellFeatureData);
  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_Volumes);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(exemplaryDataPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(DataPath({k_Volumes})));
    const DataArray<float32>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<float32>>(exemplaryDataPath);
    const DataArray<float32>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<float32>>(DataPath({k_Volumes}));
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_EquivalentDiameters);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(exemplaryDataPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(DataPath({k_EquivalentDiameters})));
    const DataArray<float32>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<float32>>(exemplaryDataPath);
    const DataArray<float32>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<float32>>(DataPath({k_EquivalentDiameters}));
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_NumElements);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(exemplaryDataPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(DataPath({k_NumElements})));

    const DataArray<int32>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<int32>>(exemplaryDataPath);
    const DataArray<int32>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<int32>>(DataPath({k_NumElements}));
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes.dream3d", unit_test::k_BinaryDir)));
}
