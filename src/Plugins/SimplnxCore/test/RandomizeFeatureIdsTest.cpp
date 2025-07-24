#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/RandomizeFeatureIdsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FeatureIdsArrayName = "Feature Ids Array";
constexpr StringLiteral k_FeatureAMName = "Feature AM";
constexpr StringLiteral k_FeatureArrayName = "Feature Array";

const DataPath k_FeatureIdsPath = DataPath{{k_FeatureIdsArrayName}};
const DataPath k_FeatureAMPath = DataPath{{k_FeatureAMName}};
const DataPath k_FeatureArrayPath = k_FeatureAMPath.createChildPath(k_FeatureArrayName);
/**
 * | Feature Id | Count |
 * |------------|-------|
 * |     0      |   4   |
 * |     1      |   2   |
 * |     2      |   3   |
 * |     3      |   1   |
 * |     4      |   5   |
 */
constexpr std::array<int32, 15> k_OriginalFeatureIds = {0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 4, 4, 4, 4, 4};
// constexpr std::array<int32, 5> k_OriginalFeatureArray = {1, 2, 3, 4, 5};
constexpr std::array<int32, 5> k_OriginalFeatureArray = {0, 1, 2, 3, 4};

void ValidateOutput(const DataStructure& dataStructure)
{
  const auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& featureArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureArrayPath);

  // Check mapping is preserved
  for(usize i = 0; i < featureIdsArray.getNumberOfTuples(); i++)
  {
    REQUIRE(featureArray[featureIdsArray[i]] == k_OriginalFeatureArray[k_OriginalFeatureIds[i]]);
  }

  // Check new array is actually sorted. Assumption: k_OriginalFeatureArray was created with an ascending sort
  usize sequentialValueCount = 0;
  for(usize i = 0; i < featureArray.getNumberOfTuples() - 1; i++)
  {
    if(featureArray[i] > featureArray[i + 1])
    {
      break;
    }
    sequentialValueCount++;
  }

  REQUIRE_FALSE(sequentialValueCount == featureArray.getNumberOfTuples() - 1);
}
} // namespace

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: 5 Feature Test", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  DataStructure dataStructure;
  {
    Int32Array* featureIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsArrayName, std::vector<usize>{15}, std::vector<usize>{1});
    (*featureIdsArray)[0] = k_OriginalFeatureIds[0];
    (*featureIdsArray)[1] = k_OriginalFeatureIds[1];
    (*featureIdsArray)[2] = k_OriginalFeatureIds[2];
    (*featureIdsArray)[3] = k_OriginalFeatureIds[3];
    (*featureIdsArray)[4] = k_OriginalFeatureIds[4];
    (*featureIdsArray)[5] = k_OriginalFeatureIds[5];
    (*featureIdsArray)[6] = k_OriginalFeatureIds[6];
    (*featureIdsArray)[7] = k_OriginalFeatureIds[7];
    (*featureIdsArray)[8] = k_OriginalFeatureIds[8];
    (*featureIdsArray)[9] = k_OriginalFeatureIds[9];
    (*featureIdsArray)[10] = k_OriginalFeatureIds[10];
    (*featureIdsArray)[11] = k_OriginalFeatureIds[11];
    (*featureIdsArray)[12] = k_OriginalFeatureIds[12];
    (*featureIdsArray)[13] = k_OriginalFeatureIds[13];
    (*featureIdsArray)[14] = k_OriginalFeatureIds[14];

    AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{5});

    Int32Array* featureArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureArrayName, featureAM->getShape(), std::vector<usize>{1}, featureAM->getId());
    (*featureArray)[0] = k_OriginalFeatureArray[0];
    (*featureArray)[1] = k_OriginalFeatureArray[1];
    (*featureArray)[2] = k_OriginalFeatureArray[2];
    (*featureArray)[3] = k_OriginalFeatureArray[3];
    (*featureArray)[4] = k_OriginalFeatureArray[4];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RandomizeFeatureIdsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ValidateOutput(dataStructure);
}

TEST_CASE("SimplnxCore::RandomizeFeatureIdsFilter: 4 Feature Test", "[SimplnxCore][RandomizeFeatureIdsFilter]")
{
  DataStructure dataStructure;
  {
    Int32Array* featureIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIdsArrayName, std::vector<usize>{10}, std::vector<usize>{1});
    (*featureIdsArray)[0] = k_OriginalFeatureIds[0];
    (*featureIdsArray)[1] = k_OriginalFeatureIds[1];
    (*featureIdsArray)[2] = k_OriginalFeatureIds[2];
    (*featureIdsArray)[3] = k_OriginalFeatureIds[3];
    (*featureIdsArray)[4] = k_OriginalFeatureIds[4];
    (*featureIdsArray)[5] = k_OriginalFeatureIds[5];
    (*featureIdsArray)[6] = k_OriginalFeatureIds[6];
    (*featureIdsArray)[7] = k_OriginalFeatureIds[7];
    (*featureIdsArray)[8] = k_OriginalFeatureIds[8];
    (*featureIdsArray)[9] = k_OriginalFeatureIds[9];

    AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{4});

    Int32Array* featureArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureArrayName, featureAM->getShape(), std::vector<usize>{1}, featureAM->getId());
    (*featureArray)[0] = k_OriginalFeatureArray[0];
    (*featureArray)[1] = k_OriginalFeatureArray[1];
    (*featureArray)[2] = k_OriginalFeatureArray[2];
    (*featureArray)[3] = k_OriginalFeatureArray[3];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    RandomizeFeatureIdsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RandomizeFeatureIdsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ValidateOutput(dataStructure);
}
