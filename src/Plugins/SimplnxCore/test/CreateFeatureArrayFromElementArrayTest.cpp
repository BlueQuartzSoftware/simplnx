#include "SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_FeatureIDs("FeatureIds");
const std::string k_Computed_CellData("Computed_CellData");

template <typename T>
void testElementArray(const DataPath& cellDataPath)
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});

  // This section creates the needed AttributeMatrix of size 1. The filter should be resizing as needed.
  {
    AttributeMatrix::Create(dataStructure, k_Computed_CellData, std::vector<usize>{1}, dataStructure.getId(smallIn100Group));
  }

  DataPath featureIdsDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData).createChildPath(k_FeatureIDs);
  DataPath computedFeatureGroupPath = smallIn100Group.createChildPath(k_Computed_CellData);
  DataPath computedFeatureArrayPath = computedFeatureGroupPath.createChildPath(cellDataPath.getTargetName());

  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(cellDataPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(computedFeatureGroupPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(cellDataPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 1 Component")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData).createChildPath(nx::core::Constants::k_ConfidenceIndex);
  testElementArray<float32>(cellDataPath);
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 3 Component")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData).createChildPath(nx::core::Constants::k_IPFColors);
  testElementArray<uint8>(cellDataPath);
}
