#include "SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_FeatureIDs("FeatureIds");
const std::string k_ComputedCellData("Computed_CellData");

template <typename T>
void TestElementArray(const DataPath& cellDataPath)
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({Constants::k_DataContainer});

  // This section creates the needed AttributeMatrix of size 1. The filter should be resizing as needed.
  {
    AttributeMatrix::Create(dataStructure, k_ComputedCellData, std::vector<usize>{1}, dataStructure.getId(smallIn100Group));
  }

  DataPath featureIdsDataPath = smallIn100Group.createChildPath(Constants::k_CellData).createChildPath(k_FeatureIDs);
  DataPath computedFeatureGroupPath = smallIn100Group.createChildPath(k_ComputedCellData);
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
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
    const DataArray<T>& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedFeatureArrayPath));
    const DataArray<T>& createdFeatureArray = dataStructure.getDataRefAs<DataArray<T>>(computedFeatureArrayPath);

    UnitTest::CompareDataArrays<T>(featureArrayExemplary, createdFeatureArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

namespace analytical_fixtures
{
constexpr StringLiteralType k_ParentDGName = "ParentGroup";
const DataPath k_ParentDGPath({k_ParentDGName});

const DataPath k_CellAMPath = k_ParentDGPath.createChildPath(Constants::k_CellData);
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(Constants::k_FeatureIds);
const DataPath k_InputCellPath = k_CellAMPath.createChildPath(Constants::k_FaceData);

const DataPath k_FeatureAMPath = k_ParentDGPath.createChildPath(Constants::k_FeatureData);
const DataPath k_OutputFeaturePath = k_FeatureAMPath.createChildPath("OutputFeatureArray");
} // namespace analytical_fixtures

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 1 Component")
{
  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(Constants::k_CellData).createChildPath(Constants::k_ConfidenceIndex);
  TestElementArray<float32>(cellDataPath);
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 3 Component")
{
  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(Constants::k_CellData).createChildPath(Constants::k_IPFColors);
  TestElementArray<uint8>(cellDataPath);
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-1 single-component consistent", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 1 (Analytical) + Class 4 (Invariant)
  // featureIds = [0, 1, 2, 1, 2]
  // cellValues (float32, 1-comp) = [5.0, 10.0, 20.0, 10.0, 20.0]
  // Hand derivation:
  //   feature 0: cell 0 only → output[0] = 5.0
  //   feature 1: cell 1 (10.0) then cell 3 (10.0) -same, no warning → output[1] = 10.0
  //   feature 2: cell 2 (20.0) then cell 4 (20.0) -same, no warning → output[2] = 20.0
  // Expected output: [5.0, 10.0, 20.0], 0 warnings
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{5ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = 0;
    (*fidsArrayPtr)[1] = 1;
    (*fidsArrayPtr)[2] = 2;
    (*fidsArrayPtr)[3] = 1;
    (*fidsArrayPtr)[4] = 2;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 5.0f;
    (*cellFloatArrayPtr)[1] = 10.0f;
    (*cellFloatArrayPtr)[2] = 20.0f;
    (*cellFloatArrayPtr)[3] = 10.0f;
    (*cellFloatArrayPtr)[4] = 20.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty()); // Class 4: 0 warnings when all features are consistent
  }

  // Validation
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath));
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath));
    const auto& outArray = dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath);

    // Class 4 invariants: shape, type, and component count
    REQUIRE(outArray.getNumberOfTuples() == 3); // max(featureIds)+1 = 2+1 = 3
    REQUIRE(outArray.getDataType() == inputArray.getDataType());
    REQUIRE(outArray.getNumberOfComponents() == inputArray.getNumberOfComponents());

    // Class 1 expected values (hand-derived)
    REQUIRE(outArray[0] == 5.0f);
    REQUIRE(outArray[1] == 10.0f);
    REQUIRE(outArray[2] == 20.0f);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-2 single-component inconsistent", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 1 (Analytical) + Class 4 (Invariant)
  // featureIds = [1, 2, 1, 2]
  // cellValues (float32, 1-comp) = [10.0, 20.0, 15.0, 20.0]
  // Hand derivation:
  //   feature 0: never written → 0.0 (CreateArrayAction fill="0")
  //   feature 1: cell 0 (10.0) then cell 2 (15.0) -differ → one warning; last-writer → output[1] = 15.0
  //   feature 2: cell 1 (20.0) then cell 3 (20.0) -same, no additional warning → output[2] = 20.0
  // Expected output: [0.0, 15.0, 20.0], exactly 1 warning
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = 1;
    (*fidsArrayPtr)[1] = 2;
    (*fidsArrayPtr)[2] = 1;
    (*fidsArrayPtr)[3] = 2;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 10.0f;
    (*cellFloatArrayPtr)[1] = 20.0f;
    (*cellFloatArrayPtr)[2] = 15.0f;
    (*cellFloatArrayPtr)[3] = 20.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().size() == 1); // Class 4: exactly 1 warning (guarded by result.warnings().empty())
  }

  // Validation
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath));
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath));
    const auto& outArray = dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath);

    // Class 4 invariants: shape, type, and component count
    REQUIRE(outArray.getNumberOfTuples() == 3); // max(featureIds)+1 = 2+1 = 3
    REQUIRE(outArray.getDataType() == inputArray.getDataType());
    REQUIRE(outArray.getNumberOfComponents() == inputArray.getNumberOfComponents());

    // Class 1 expected values (hand-derived)
    REQUIRE(outArray[0] == 0.0f);  // feature 0: never written; fill value = "0"
    REQUIRE(outArray[1] == 15.0f); // feature 1: last-writer cell 2 wins (15.0 over first-seen 10.0)
    REQUIRE(outArray[2] == 20.0f); // feature 2: consistent

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-3 three-component consistent", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 1 (Analytical) + Class 4 (Invariant)
  // featureIds = [1, 2, 1, 2]
  // cellValues (uint8, 3-comp): cell0=[10,20,30], cell1=[40,50,60], cell2=[10,20,30], cell3=[40,50,60]
  // Hand derivation:
  //   feature 0: never written → [0, 0, 0]
  //   feature 1: cell 0 [10,20,30] then cell 2 [10,20,30] -same per component → no warning → output[1] = [10, 20, 30]
  //   feature 2: cell 1 [40,50,60] then cell 3 [40,50,60] -same per component → no warning → output[2] = [40, 50, 60]
  // Expected output: [[0,0,0], [10,20,30], [40,50,60]], 0 warnings
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = 1;
    (*fidsArrayPtr)[1] = 2;
    (*fidsArrayPtr)[2] = 1;
    (*fidsArrayPtr)[3] = 2;

    auto* cellRgbArrayPtr =
        UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{3ULL}, cellAmPtr->getId());
    // cell 0 → [10, 20, 30]
    (*cellRgbArrayPtr)[0 * 3 + 0] = 10;
    (*cellRgbArrayPtr)[0 * 3 + 1] = 20;
    (*cellRgbArrayPtr)[0 * 3 + 2] = 30;
    // cell 1 → [40, 50, 60]
    (*cellRgbArrayPtr)[1 * 3 + 0] = 40;
    (*cellRgbArrayPtr)[1 * 3 + 1] = 50;
    (*cellRgbArrayPtr)[1 * 3 + 2] = 60;
    // cell 2 → [10, 20, 30]
    (*cellRgbArrayPtr)[2 * 3 + 0] = 10;
    (*cellRgbArrayPtr)[2 * 3 + 1] = 20;
    (*cellRgbArrayPtr)[2 * 3 + 2] = 30;
    // cell 3 → [40, 50, 60]
    (*cellRgbArrayPtr)[3 * 3 + 0] = 40;
    (*cellRgbArrayPtr)[3 * 3 + 1] = 50;
    (*cellRgbArrayPtr)[3 * 3 + 2] = 60;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty()); // Class 4: 0 warnings when all features are consistent
  }

  // Validation
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath));
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(analytical_fixtures::k_OutputFeaturePath));
    const auto& outArray = dataStructure.getDataRefAs<UInt8Array>(analytical_fixtures::k_OutputFeaturePath);

    // Class 4 invariants: shape, type, and component count
    REQUIRE(outArray.getNumberOfTuples() == 3); // max(featureIds)+1 = 2+1 = 3
    REQUIRE(outArray.getDataType() == inputArray.getDataType());
    REQUIRE(outArray.getNumberOfComponents() == inputArray.getNumberOfComponents());

    // Class 1 expected values (hand-derived)
    // feature 0: never written → fill = [0, 0, 0]
    REQUIRE(outArray[0 * 3 + 0] == 0);
    REQUIRE(outArray[0 * 3 + 1] == 0);
    REQUIRE(outArray[0 * 3 + 2] == 0);
    // feature 1: cells 0 and 2 both [10, 20, 30]
    REQUIRE(outArray[1 * 3 + 0] == 10);
    REQUIRE(outArray[1 * 3 + 1] == 20);
    REQUIRE(outArray[1 * 3 + 2] == 30);
    // feature 2: cells 1 and 3 both [40, 50, 60]
    REQUIRE(outArray[2 * 3 + 0] == 40);
    REQUIRE(outArray[2 * 3 + 1] == 50);
    REQUIRE(outArray[2 * 3 + 2] == 60);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-4 error path all-negative featureIds", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 4 (Invariant)
  // featureIds = [-1, -2, -1] -all negative; std::max_element returns -1
  // maxValue = -1 < 0 → MakeErrorResult(-81880, ...)
  // Expected: preflight succeeds; execute fails with error code -81880
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{3ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{3ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = -1;
    (*fidsArrayPtr)[1] = -2;
    (*fidsArrayPtr)[2] = -1;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 1.0f;
    (*cellFloatArrayPtr)[1] = 2.0f;
    (*cellFloatArrayPtr)[2] = 1.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE_FALSE(executeResult.result.valid());
    REQUIRE(executeResult.result.errors()[0].code == -81880);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-5 error path shrink-protection guard", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 4 (Invariant)
  // Feature AM: 2 tuples; SiblingArray child created directly with 5 tuples (AM tuple count not cascaded)
  // featureIds = [1, 2, 3, 2] → maxValue = 3, maxValue+1 = 4
  // 4 > AM.tupleCount=2 → outer grow condition fires
  // SiblingArray.getNumberOfTuples()=5 > 4 → shrink-protection inner check fires → MakeErrorResult(-81881, ...)
  // Expected: preflight succeeds; execute fails with error code -81881
  DataStructure dataStructure;
  const DataPath kSiblingArrayPath = analytical_fixtures::k_FeatureAMPath.createChildPath("SiblingArray");

  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    auto* featureAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{2ULL}, topGroupPtr->getId());

    // Child array created directly with 5 tuples -more than AM.tupleCount=2 and more than maxValue+1=4
    auto* siblingArrayPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, kSiblingArrayPath.getTargetName(), featureAmPtr->getShape(), ShapeType{1ULL}, featureAmPtr->getId());
    auto siblingArrayResizeResult = siblingArrayPtr->resizeTuples(ShapeType{5ULL});
    SIMPLNX_RESULT_REQUIRE_VALID(siblingArrayResizeResult);

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = 1;
    (*fidsArrayPtr)[1] = 2;
    (*fidsArrayPtr)[2] = 3;
    (*fidsArrayPtr)[3] = 2;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 10.0f;
    (*cellFloatArrayPtr)[1] = 20.0f;
    (*cellFloatArrayPtr)[2] = 30.0f;
    (*cellFloatArrayPtr)[3] = 20.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE_FALSE(executeResult.result.valid());
    REQUIRE(executeResult.result.errors()[0].code == -81881);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-6 gap in FeatureIds range (resize-grown tuple never written)",
          "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 1 (Analytical) + Class 4 (Invariant)
  // featureIds = [0, 2, 0, 2] -feature 1 is in [0..max] but no cell maps to it
  // cellValues (float32, 1-comp) = [5.0, 30.0, 5.0, 30.0]
  // Feature AM starts with 1 tuple; resizeTuples grows to 3, adding indices 1 and 2.
  // Hand derivation:
  //   feature 0: cell 0 (5.0) then cell 2 (5.0) -same, no warning → output[0] = 5.0
  //   feature 1: gap -grown by resizeTuples, never written; m_InitValue=0 (in-core DataStore<T>) → output[1] = 0.0
  //   feature 2: cell 1 (30.0) then cell 3 (30.0) -same, no warning → output[2] = 30.0
  // Expected output: [5.0, 0.0, 30.0], 0 warnings
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = 0;
    (*fidsArrayPtr)[1] = 2;
    (*fidsArrayPtr)[2] = 0;
    (*fidsArrayPtr)[3] = 2;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 5.0f;
    (*cellFloatArrayPtr)[1] = 30.0f;
    (*cellFloatArrayPtr)[2] = 5.0f;
    (*cellFloatArrayPtr)[3] = 30.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(executeResult.result.warnings().empty()); // Class 4: 0 warnings when all features are consistent
  }

  // Validation
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath));
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(analytical_fixtures::k_InputCellPath);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath));
    const auto& outArray = dataStructure.getDataRefAs<Float32Array>(analytical_fixtures::k_OutputFeaturePath);

    // Class 4 invariants: shape, type, and component count
    REQUIRE(outArray.getNumberOfTuples() == 3); // max(featureIds)+1 = 2+1 = 3
    REQUIRE(outArray.getDataType() == inputArray.getDataType());
    REQUIRE(outArray.getNumberOfComponents() == inputArray.getNumberOfComponents());

    // Class 1 expected values (hand-derived)
    REQUIRE(outArray[0] == 5.0f);  // feature 0: written by cells 0 and 2
    REQUIRE(outArray[1] == 0.0f);  // feature 1: gap, grown by resizeTuples, never written; 0 from m_InitValue (in-core DataStore<T>)
    REQUIRE(outArray[2] == 30.0f); // feature 2: written by cells 1 and 3

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-7 error path empty featureIds array", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 4 (Invariant)
  // featureIds = [] (0 tuples) -std::max_element on empty range returns end;
  // accessing featureIdsRef[distance(begin,end)=0] on a 0-element store is UB without the guard.
  // Expected: preflight succeeds; execute fails with error code -81882
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{0ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE_FALSE(executeResult.result.valid());
    REQUIRE(executeResult.result.errors()[0].code == -81882);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-8 error path mixed negative and positive featureIds",
          "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // Oracle class: Class 4 (Invariant)
  // featureIds = [-1, 1, 2, -1] -maxValue=2 (positive) passes the old all-negative guard,
  // but featureIdx=-1 in the copy loop converts to usize(UINT64_MAX) → OOB write without the min check.
  // Expected: preflight succeeds; execute fails with error code -81880
  DataStructure dataStructure;
  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr =
        Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*fidsArrayPtr)[0] = -1;
    (*fidsArrayPtr)[1] = 1;
    (*fidsArrayPtr)[2] = 2;
    (*fidsArrayPtr)[3] = -1;

    auto* cellFloatArrayPtr =
        Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
    (*cellFloatArrayPtr)[0] = 1.0f;
    (*cellFloatArrayPtr)[1] = 2.0f;
    (*cellFloatArrayPtr)[2] = 3.0f;
    (*cellFloatArrayPtr)[3] = 1.0f;
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE_FALSE(executeResult.result.valid());
    REQUIRE(executeResult.result.errors()[0].code == -81880);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-9 error path featureIds tuple count mismatch", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][AnalyticalFixtures]")
{
  // The cell array has four tuples. FeatureIds has two tuples in a separate AttributeMatrix.
  // Preflight must reject the mismatch before execution reads featureIds[2] or featureIds[3].
  // The expected error code is -81883.
  DataStructure dataStructure;
  const DataPath kSmallerAmPath = analytical_fixtures::k_ParentDGPath.createChildPath("SmallerAM");
  const DataPath kMismatchedFeatureIdsPath = kSmallerAmPath.createChildPath(Constants::k_FeatureIds);

  // Creation
  {
    auto* topGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{4ULL}, topGroupPtr->getId());
    auto* smallerAmPtr = AttributeMatrix::Create(dataStructure, kSmallerAmPath.getTargetName(), ShapeType{2ULL}, topGroupPtr->getId());
    AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1ULL}, topGroupPtr->getId());

    auto* fidsArrayPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, kMismatchedFeatureIdsPath.getTargetName(), smallerAmPtr->getShape(), ShapeType{1ULL}, smallerAmPtr->getId());
    (*fidsArrayPtr)[0] = 1;
    (*fidsArrayPtr)[1] = 2;

    Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1ULL}, cellAmPtr->getId());
  }

  // Execution
  {
    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args;
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kMismatchedFeatureIdsPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE_FALSE(preflightResult.outputActions.valid());
    REQUIRE(preflightResult.outputActions.errors()[0].code == -81883);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Bulk last-value boundary oracle", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CellCount = 65538;
  constexpr usize k_ComponentCount = 3;
  DataStructure dataStructure;
  auto* parentPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_ParentDGName);
  REQUIRE(parentPtr != nullptr);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_CellAMPath.getTargetName(), ShapeType{k_CellCount}, parentPtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureAMPath.getTargetName(), ShapeType{1}, parentPtr->getId());
  REQUIRE(featureAmPtr != nullptr);

  auto* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_FeatureIdsPath.getTargetName(), cellAmPtr->getShape(), ShapeType{1}, cellAmPtr->getId());
  auto* valuesPtr =
      Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, analytical_fixtures::k_InputCellPath.getTargetName(), cellAmPtr->getShape(), ShapeType{k_ComponentCount}, cellAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  REQUIRE(valuesPtr != nullptr);
  for(usize cellIdx = 0; cellIdx < k_CellCount; cellIdx++)
  {
    (*featureIdsPtr)[cellIdx] = 0;
    for(usize compIdx = 0; compIdx < k_ComponentCount; compIdx++)
    {
      (*valuesPtr)[cellIdx * k_ComponentCount + compIdx] = 0;
    }
  }
  // Feature 3 receives a conflicting value in the second page.
  (*featureIdsPtr)[0] = 3;
  (*valuesPtr)[0] = 1;
  (*valuesPtr)[1] = 10;
  (*valuesPtr)[2] = 100;
  (*featureIdsPtr)[65536] = 3;
  (*valuesPtr)[65536 * k_ComponentCount] = 2;
  (*valuesPtr)[65536 * k_ComponentCount + 1] = 20;
  (*valuesPtr)[65536 * k_ComponentCount + 2] = 200;
  // Feature 1 first appears in the final tuple. Feature 2 is a gap.
  (*featureIdsPtr)[65537] = 1;
  (*valuesPtr)[65537 * k_ComponentCount] = 7;
  (*valuesPtr)[65537 * k_ComponentCount + 1] = 70;
  (*valuesPtr)[65537 * k_ComponentCount + 2] = 700;

  CreateFeatureArrayFromElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_InputCellPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureIdsPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(analytical_fixtures::k_FeatureAMPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(analytical_fixtures::k_OutputFeaturePath.getTargetName()));

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings().front().code == -1000);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(analytical_fixtures::k_OutputFeaturePath));
  const auto& output = dataStructure.getDataRefAs<Int32Array>(analytical_fixtures::k_OutputFeaturePath);
  REQUIRE(output.getNumberOfTuples() == 4);
  REQUIRE(output.getNumberOfComponents() == k_ComponentCount);
  REQUIRE(output[0] == 0);
  REQUIRE(output[1] == 0);
  REQUIRE(output[2] == 0);
  REQUIRE(output[3] == 7);
  REQUIRE(output[4] == 70);
  REQUIRE(output[5] == 700);
  REQUIRE(output[6] == 0);
  REQUIRE(output[7] == 0);
  REQUIRE(output[8] == 0);
  REQUIRE(output[9] == 2);
  REQUIRE(output[10] == 20);
  REQUIRE(output[11] == 200);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateFeatureArrayFromElementArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateFeatureArrayFromElementArrayFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<CreateFeatureArrayFromElementArrayFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key) == "TestName");
    }
  }
}
