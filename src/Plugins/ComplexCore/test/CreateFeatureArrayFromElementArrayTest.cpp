#include "ComplexCore/Filters/CreateFeatureArrayFromElementArray.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const std::string k_FeatureIDs("FeatureIds");
const std::string k_Computed_CellData("Computed_CellData");

template <typename T>
void testElementArray(const DataPath& cellDataPath)
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_test_data_1.tar.gz", "6_5_test_data_1");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1/6_5_test_data_1.dream3d", complex::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});

  {
    auto* computedFeatureData = AttributeMatrix::Create(dataStructure, k_Computed_CellData, std::vector<usize>{81}, dataStructure.getId(smallIn100Group));
  }

  DataPath featureIdsDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData).createChildPath(k_FeatureIDs);
  DataPath computedFeatureGroupPath = smallIn100Group.createChildPath(k_Computed_CellData);
  DataPath computedFeatureArrayPath = computedFeatureGroupPath.createChildPath(cellDataPath.getTargetName());

  {
    CreateFeatureArrayFromElementArray filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateFeatureArrayFromElementArray::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(cellDataPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArray::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArray::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(computedFeatureGroupPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArray::k_CreatedArrayName_Key, std::make_any<std::string>(cellDataPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    DataPath exemplaryDataPath = smallIn100Group.createChildPath("CellFeatureData").createChildPath(cellDataPath.getTargetName());
    const DataArray<T>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);

    const DataArray<T>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<T>>(computedFeatureArrayPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      auto oldVal = featureArrayExemplary[i];
      auto newVal = createdFeatureArray[i];
      if(oldVal != newVal)
      {
        REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
        break;
      }
    }
  }
}
} // namespace

TEST_CASE("ComplexCore::CreateFeatureArrayFromElementArray: Valid filter execution - 1 Component")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData).createChildPath(complex::Constants::k_ConfidenceIndex);
  testElementArray<float32>(cellDataPath);
}

TEST_CASE("ComplexCore::CreateFeatureArrayFromElementArray: Valid filter execution - 3 Component")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData).createChildPath(complex::Constants::k_IPFColors);
  testElementArray<uint8>(cellDataPath);
}
