#include "SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArray.hpp"
#include "SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @class CopyFeatureFailingReadStore
 * @brief Injects a selected error into a selected bulk read.
 * @tparam T Specifies the store element type.
 */
template <typename T>
class CopyFeatureFailingReadStore : public DataStore<T>
{
public:
  /**
   * @brief Creates an in-memory store with a selected read failure.
   * @param tupleShape Store tuple shape.
   * @param componentShape Store component shape.
   * @param value Optional initialization value.
   * @param errorCode Error code returned by the selected read.
   * @param failOnRead One-based read call that returns the error.
   */
  CopyFeatureFailingReadStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode, usize failOnRead = 1)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  , m_FailOnRead(failOnRead)
  {
  }

  /**
   * @brief Reads values until the selected call returns the injected error.
   * @param offset Zero-based first source element.
   * @param buffer Receives values on successful calls.
   * @return The injected error or the underlying DataStore result.
   */
  Result<> copyIntoBuffer(usize offset, nonstd::span<T> buffer) const override
  {
    if(++m_ReadCount == m_FailOnRead)
    {
      return MakeErrorResult(m_ErrorCode, "Injected CopyFeatureArray bulk-read failure");
    }
    return DataStore<T>::copyIntoBuffer(offset, buffer);
  }

private:
  int32 m_ErrorCode;
  usize m_FailOnRead;
  mutable usize m_ReadCount = 0;
};

/**
 * @class CopyFeatureFailingWriteStore
 * @brief Injects a selected error into every bulk write.
 * @tparam T Specifies the store element type.
 */
template <typename T>
class CopyFeatureFailingWriteStore : public DataStore<T>
{
public:
  /**
   * @brief Creates an in-memory store with a selected write error.
   * @param tupleShape Store tuple shape.
   * @param componentShape Store component shape.
   * @param value Optional initialization value.
   * @param errorCode Error code returned by bulk writes.
   */
  CopyFeatureFailingWriteStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected CopyFeatureArray bulk-write failure");
  }

private:
  int32 m_ErrorCode;
};

/**
 * @class CopyFeatureCancelAfterSecondReadStore
 * @brief Requests cancellation after the second successful bulk read.
 */
class CopyFeatureCancelAfterSecondReadStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates a scalar int32 store that updates a caller-owned cancel flag.
   * @param tupleShape Store tuple shape.
   * @param shouldCancel Cancel flag that must outlive this store.
   */
  CopyFeatureCancelAfterSecondReadStore(const ShapeType& tupleShape, std::atomic_bool& shouldCancel)
  : DataStore<int32>(tupleShape, ShapeType{1}, int32{0})
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Performs a bulk read and requests cancellation after the second success.
   * @param offset Zero-based first source element.
   * @param buffer Receives the selected values.
   * @return The underlying DataStore result.
   */
  Result<> copyIntoBuffer(usize offset, nonstd::span<int32> buffer) const override
  {
    auto result = DataStore<int32>::copyIntoBuffer(offset, buffer);
    if(result.valid() && ++m_ReadCount == 2)
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable usize m_ReadCount = 0;
};

const std::string k_CellFeatureIdsArrayName("FeatureIds");
const std::string k_FeatureTemperatureName("Feature Temperature");
const std::string k_FeatureDataArrayName("Feature Data Array");
const std::string k_CellTempArraySuffix("_ToCell");
const DataPath k_CellTempArrayPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
const DataPath k_CellFeatureArrayPath({k_FeatureDataArrayName + k_CellTempArraySuffix});

/**
 * @namespace AnalyticalFixtures
 * @brief Provides hand-derived input and expected arrays for the analytical oracle.
 *
 * The V&V report documents the complete oracle derivation.
 */
namespace AnalyticalFixtures
{
const std::string k_ImageGeometryName("Image Geometry");
const std::string k_CellDataName("Cell Data");
const std::string k_CellFeatureDataName("Cell Feature Data");
const std::string k_AvgTempName("AvgTemp");
const std::string k_RGBName("RGB");
const std::string k_ActiveName("Active");
const std::string k_Suffix("_Cell");

constexpr usize k_RGBComponentCount = 3;

const DataPath k_FeatureIdsPath({k_ImageGeometryName, k_CellDataName, k_CellFeatureIdsArrayName});
const DataPath k_FeatureAMPath({k_ImageGeometryName, k_CellFeatureDataName});
const DataPath k_AvgTempPath({k_ImageGeometryName, k_CellFeatureDataName, k_AvgTempName});
const DataPath k_RGBPath({k_ImageGeometryName, k_CellFeatureDataName, k_RGBName});
const DataPath k_ActivePath({k_ImageGeometryName, k_CellFeatureDataName, k_ActiveName});
const DataPath k_AvgTempCellPath({k_ImageGeometryName, k_CellDataName, k_AvgTempName + k_Suffix});
const DataPath k_RGBCellPath({k_ImageGeometryName, k_CellDataName, k_RGBName + k_Suffix});
const DataPath k_ActiveCellPath({k_ImageGeometryName, k_CellDataName, k_ActiveName + k_Suffix});

// The 4 by 3 by 1 geometry has 12 cells and feature identifiers 0 through 3.
const std::vector<int32> k_FeatureIds = {0, 1, 1, 2, 2, 0, 3, 1, 3, 3, 0, 2};

// These arrays contain four feature-level source tuples.
const std::vector<float32> k_AvgTemp = {10.5F, 20.25F, -30.75F, 40.125F};
const std::vector<int32> k_RGB = {1, 2, 3, 40, 50, 60, -7, 8, -9, 100, 200, 127};
const std::vector<bool> k_Active = {false, true, true, false};

// Each output component uses `source[FeatureIds[i] * C + c]`.
// The expected scalar output uses AvgTemp at each cell's feature identifier.
const std::vector<float32> k_ExpectedAvgTempCell = {10.5F, 20.25F, 20.25F, -30.75F, -30.75F, 10.5F, 40.125F, 20.25F, 40.125F, 40.125F, 10.5F, -30.75F};

// Each expected RGB tuple copies the three components from its feature tuple.
const std::vector<int32> k_ExpectedRGBCell = {
    1, 2, 3, 40, 50, 60, 40, 50, 60, -7, 8, -9, -7, 8, -9, 1, 2, 3, 100, 200, 127, 40, 50, 60, 100, 200, 127, 100, 200, 127, 1, 2, 3, -7, 8, -9,
};

// Each expected Active value copies the value from its feature tuple.
const std::vector<bool> k_ExpectedActiveCell = {false, true, true, true, true, false, false, true, false, false, false, true};

/**
 * @brief Builds the 4 by 3 by 1 analytical fixture and its feature arrays.
 * @return The populated DataStructure.
 */
DataStructure CreateFixture()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  imageGeomPtr->setDimensions(SizeVec3{4, 3, 1});
  imageGeomPtr->setSpacing(FloatVec3{1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin(FloatVec3{0.0F, 0.0F, 0.0F});

  // The AttributeMatrix tuple shape uses slowest-to-fastest {Z, Y, X} order.
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, k_CellDataName, std::vector<usize>{1, 3, 4}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);

  auto* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {1, 3, 4}, {1}, cellAMPtr->getId());
  auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();
  for(usize i = 0; i < k_FeatureIds.size(); i++)
  {
    featureIdsStoreRef[i] = k_FeatureIds[i];
  }

  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_CellFeatureDataName, std::vector<usize>{4}, imageGeomPtr->getId());

  auto* avgTempPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_AvgTempName, {4}, {1}, featureAMPtr->getId());
  auto& avgTempStoreRef = avgTempPtr->getDataStoreRef();
  for(usize i = 0; i < k_AvgTemp.size(); i++)
  {
    avgTempStoreRef[i] = k_AvgTemp[i];
  }

  auto* rgbPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_RGBName, {4}, {k_RGBComponentCount}, featureAMPtr->getId());
  auto& rgbStoreRef = rgbPtr->getDataStoreRef();
  for(usize i = 0; i < k_RGB.size(); i++)
  {
    rgbStoreRef[i] = k_RGB[i];
  }

  auto* activePtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_ActiveName, {4}, {1}, featureAMPtr->getId());
  auto& activeStoreRef = activePtr->getDataStoreRef();
  for(usize i = 0; i < k_Active.size(); i++)
  {
    activeStoreRef[i] = k_Active[i];
  }

  return dataStructure;
}

/**
 * @brief Creates filter arguments for every analytical feature source array.
 * @return Configured analytical-test arguments.
 */
Arguments CreateArguments()
{
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_AvgTempPath, k_RGBPath, k_ActivePath}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_Suffix));
  return args;
}

} // namespace AnalyticalFixtures
} // namespace

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Empty selection (filter guard)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  CopyFeatureArrayToElementArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // A valid FeatureIds path lets the filter's empty-selection guard produce the error.
  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {30}, {1});

  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(""));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  for(const Error& err : preflightResult.outputActions.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors().size() == 1);
  for(const Error& err : executeResult.result.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Feature array tuple count mismatch (-3020)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  // FeatureIds must exist, but it is not part of the feature-array tuple-count check.
  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {30}, {1});

  // Selected feature arrays with three and four tuples reach error -3020 in validateNumberOfTuples().
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {3}, {1});
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureDataArrayName, {4}, {1});

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName}), DataPath({k_FeatureDataArrayName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3020);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Non-DataArray selection rejected", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // NeighborList implements IArray but not IDataArray.
  // Parameter validation must reject it before preflight can request an IDataArray reference.
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureAMPath));
  auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureAMPath);
  auto* neighborListPtr = NeighborList<float32>::Create(dataStructure, "NeighborList", featureAM.getShape(), featureAM.getId());
  REQUIRE(neighborListPtr != nullptr);

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{AnalyticalFixtures::k_FeatureAMPath.createChildPath("NeighborList")}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Preflight Error - Suffix contains '/' (-3021)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>("/Cell"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3021);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - Created name collides with existing array (-266)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // An empty suffix makes the created path equal the selected FeatureIds path.
  // Direct filter preflight does not apply output actions, so execution returns collision error -266.
  // Pipeline preflight applies actions and detects the same collision before execution.
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{AnalyticalFixtures::k_FeatureIdsPath}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(""));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -266);

  // The failed create action must leave the original FeatureIds values unchanged.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  const auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  for(usize i = 0; i < AnalyticalFixtures::k_FeatureIds.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(featureIdsStoreRef[i] == AnalyticalFixtures::k_FeatureIds[i]);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Analytical Oracle (Class 1)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const usize numCells = AnalyticalFixtures::k_FeatureIds.size();

  // The analytical oracle compares each output with hand-derived constants.
  // The V&V report derives `out[i * C + c] = source[FeatureIds[i] * C + c]`.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath));
  const auto& avgTempCellRef = dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath).getDataStoreRef();
  REQUIRE(avgTempCellRef.getNumberOfTuples() == numCells);
  REQUIRE(avgTempCellRef.getNumberOfComponents() == 1);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedAvgTempCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(avgTempCellRef[i] == AnalyticalFixtures::k_ExpectedAvgTempCell[i]);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath));
  const auto& rgbCellRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath).getDataStoreRef();
  REQUIRE(rgbCellRef.getNumberOfTuples() == numCells);
  REQUIRE(rgbCellRef.getNumberOfComponents() == AnalyticalFixtures::k_RGBComponentCount);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedRGBCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(rgbCellRef[i] == AnalyticalFixtures::k_ExpectedRGBCell[i]);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath));
  const auto& activeCellRef = dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath).getDataStoreRef();
  REQUIRE(activeCellRef.getNumberOfTuples() == numCells);
  REQUIRE(activeCellRef.getNumberOfComponents() == 1);
  for(usize i = 0; i < AnalyticalFixtures::k_ExpectedActiveCell.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(activeCellRef[i] == AnalyticalFixtures::k_ExpectedActiveCell[i]);
  }

  // Cells with the same feature identifier must have equal output tuples.
  // The three-component output checks this piecewise-constancy invariant.
  for(usize i = 0; i < numCells; i++)
  {
    for(usize j = i + 1; j < numCells; j++)
    {
      if(AnalyticalFixtures::k_FeatureIds[i] == AnalyticalFixtures::k_FeatureIds[j])
      {
        for(usize c = 0; c < AnalyticalFixtures::k_RGBComponentCount; c++)
        {
          CAPTURE(i, j, c);
          REQUIRE(rgbCellRef[i * AnalyticalFixtures::k_RGBComponentCount + c] == rgbCellRef[j * AnalyticalFixtures::k_RGBComponentCount + c]);
        }
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Scanline propagates bulk failures before partial writes", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  auto makeValues = [] {
    CopyFeatureArrayToElementArrayInputValues values;
    values.SelectedFeatureArrayPaths = {AnalyticalFixtures::k_AvgTempPath};
    values.FeatureIdsPath = AnalyticalFixtures::k_FeatureIdsPath;
    values.CreatedArraySuffix = AnalyticalFixtures::k_Suffix;
    return values;
  };
  auto addSentinelOutput = [](DataStructure& dataStructure) {
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
    auto* output = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, AnalyticalFixtures::k_AvgTempCellPath.getTargetName(), {1, 3, 4}, {1}, cellData.getId());
    if(output != nullptr)
    {
      output->fill(-42.0F);
    }
    return output;
  };

  SECTION("validator FeatureIds read fails")
  {
    DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
    dataStructure.removeData(AnalyticalFixtures::k_FeatureIdsPath);
    auto store = std::make_shared<CopyFeatureFailingReadStore<int32>>(ShapeType{1, 3, 4}, ShapeType{1}, int32{0}, -8101);
    REQUIRE(Int32Array::Create(dataStructure, k_CellFeatureIdsArrayName, store, cellData.getId()) != nullptr);
    auto* output = addSentinelOutput(dataStructure);
    REQUIRE(output != nullptr);
    std::atomic_bool shouldCancel = false;
    ForceOocAlgorithmGuard guard(true);
    auto values = makeValues();
    auto result = CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result)
    REQUIRE(result.errors()[0].code == -8101);
    REQUIRE(output->getValue(0) == -42.0F);
  }

  SECTION("FeatureIds transfer read fails after validation")
  {
    DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
    dataStructure.removeData(AnalyticalFixtures::k_FeatureIdsPath);
    auto store = std::make_shared<CopyFeatureFailingReadStore<int32>>(ShapeType{1, 3, 4}, ShapeType{1}, int32{0}, -8102, 2);
    SIMPLNX_RESULT_REQUIRE_VALID(store->copyFromBuffer(0, nonstd::span<const int32>(AnalyticalFixtures::k_FeatureIds.data(), AnalyticalFixtures::k_FeatureIds.size())));
    REQUIRE(Int32Array::Create(dataStructure, k_CellFeatureIdsArrayName, store, cellData.getId()) != nullptr);
    auto* output = addSentinelOutput(dataStructure);
    REQUIRE(output != nullptr);
    std::atomic_bool shouldCancel = false;
    ForceOocAlgorithmGuard guard(true);
    auto values = makeValues();
    auto result = CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result)
    REQUIRE(result.errors()[0].code == -8102);
    REQUIRE(output->getValue(0) == -42.0F);
  }

  SECTION("feature cache read fails")
  {
    DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
    const auto& featureData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_AvgTempPath.getParent());
    dataStructure.removeData(AnalyticalFixtures::k_AvgTempPath);
    auto store = std::make_shared<CopyFeatureFailingReadStore<float32>>(ShapeType{4}, ShapeType{1}, float32{0.0F}, -8103);
    REQUIRE(Float32Array::Create(dataStructure, AnalyticalFixtures::k_AvgTempName, store, featureData.getId()) != nullptr);
    auto* output = addSentinelOutput(dataStructure);
    REQUIRE(output != nullptr);
    std::atomic_bool shouldCancel = false;
    ForceOocAlgorithmGuard guard(true);
    auto values = makeValues();
    auto result = CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result)
    REQUIRE(result.errors()[0].code == -8103);
    REQUIRE(output->getValue(0) == -42.0F);
  }

  SECTION("output write fails")
  {
    DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
    auto store = std::make_shared<CopyFeatureFailingWriteStore<float32>>(ShapeType{1, 3, 4}, ShapeType{1}, float32{0.0F}, -8104);
    auto* output = Float32Array::Create(dataStructure, AnalyticalFixtures::k_AvgTempCellPath.getTargetName(), store, cellData.getId());
    REQUIRE(output != nullptr);
    std::atomic_bool shouldCancel = false;
    ForceOocAlgorithmGuard guard(true);
    auto values = makeValues();
    auto result = CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    SIMPLNX_RESULT_REQUIRE_INVALID(result)
    REQUIRE(result.errors()[0].code == -8104);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Pre-cancelled Scanline does not write", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
  const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
  auto* output = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, AnalyticalFixtures::k_AvgTempCellPath.getTargetName(), {1, 3, 4}, {1}, cellData.getId());
  REQUIRE(output != nullptr);
  output->fill(-42.0F);
  CopyFeatureArrayToElementArrayInputValues values;
  values.SelectedFeatureArrayPaths = {AnalyticalFixtures::k_AvgTempPath};
  values.FeatureIdsPath = AnalyticalFixtures::k_FeatureIdsPath;
  values.CreatedArraySuffix = AnalyticalFixtures::k_Suffix;
  std::atomic_bool shouldCancel = true;
  ForceOocAlgorithmGuard guard(true);
  SIMPLNX_RESULT_REQUIRE_VALID(CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)())
  for(usize i = 0; i < output->getNumberOfTuples(); i++)
  {
    REQUIRE(output->getValue(i) == -42.0F);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Cancellation after FeatureIds chunk read does not write", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();
  const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(AnalyticalFixtures::k_FeatureIdsPath.getParent());
  std::atomic_bool shouldCancel = false;
  dataStructure.removeData(AnalyticalFixtures::k_FeatureIdsPath);
  auto idsStore = std::make_shared<CopyFeatureCancelAfterSecondReadStore>(ShapeType{1, 3, 4}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(idsStore->copyFromBuffer(0, nonstd::span<const int32>(AnalyticalFixtures::k_FeatureIds.data(), AnalyticalFixtures::k_FeatureIds.size())));
  REQUIRE(Int32Array::Create(dataStructure, k_CellFeatureIdsArrayName, idsStore, cellData.getId()) != nullptr);
  auto* output = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, AnalyticalFixtures::k_AvgTempCellPath.getTargetName(), {1, 3, 4}, {1}, cellData.getId());
  REQUIRE(output != nullptr);
  output->fill(-42.0F);
  CopyFeatureArrayToElementArrayInputValues values;
  values.SelectedFeatureArrayPaths = {AnalyticalFixtures::k_AvgTempPath};
  values.FeatureIdsPath = AnalyticalFixtures::k_FeatureIdsPath;
  values.CreatedArraySuffix = AnalyticalFixtures::k_Suffix;
  ForceOocAlgorithmGuard guard(true);
  SIMPLNX_RESULT_REQUIRE_VALID(CopyFeatureArrayToElementArray(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)())
  REQUIRE(shouldCancel);
  for(usize i = 0; i < output->getNumberOfTuples(); i++)
  {
    REQUIRE(output->getValue(i) == -42.0F);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - Negative FeatureIds (-5355)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  // Preflight does not inspect feature identifier values.
  // Execution must reject a negative value through ValidateFeatureIdsToFeatureAttributeMatrixIndexing().
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  featureIdsStoreRef[5] = -1;

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -5355);
  const auto& scalarOutput = dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath);
  const auto& rgbOutput = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath);
  const auto& boolOutput = dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath);
  for(usize i = 0; i < scalarOutput.getSize(); i++)
  {
    REQUIRE(scalarOutput.getValue(i) == 0.0F);
  }
  for(usize i = 0; i < rgbOutput.getSize(); i++)
  {
    REQUIRE(rgbOutput.getValue(i) == int32{0});
  }
  for(usize i = 0; i < boolOutput.getSize(); i++)
  {
    REQUIRE(boolOutput.getValue(i) == false);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Execute Error - FeatureId exceeds Feature tuple count (-5351)", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = AnalyticalFixtures::CreateFixture();

  // Feature identifier 4 is outside the four-tuple source range.
  // Preflight accepts the shapes, but execution must return error -5351 before a read.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath));
  auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_FeatureIdsPath).getDataStoreRef();
  featureIdsStoreRef[5] = 4;

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args = AnalyticalFixtures::CreateArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == -5351);
  const auto& scalarOutput = dataStructure.getDataRefAs<Float32Array>(AnalyticalFixtures::k_AvgTempCellPath);
  const auto& rgbOutput = dataStructure.getDataRefAs<Int32Array>(AnalyticalFixtures::k_RGBCellPath);
  const auto& boolOutput = dataStructure.getDataRefAs<BoolArray>(AnalyticalFixtures::k_ActiveCellPath);
  for(usize i = 0; i < scalarOutput.getSize(); i++)
  {
    REQUIRE(scalarOutput.getValue(i) == 0.0F);
  }
  for(usize i = 0; i < rgbOutput.getSize(); i++)
  {
    REQUIRE(rgbOutput.getValue(i) == int32{0});
  }
  for(usize i = 0; i < boolOutput.getSize(); i++)
  {
    REQUIRE(boolOutput.getValue(i) == false);
  }
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Over-provisioned Feature array accepted", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // DREAM3D 6.5.171 rejects source tuples beyond the largest referenced feature identifier.
  // SIMPLNX accepts these unused feature tuples.
  DataStructure dataStructure;

  // Six cells reference features 0 through 2, while the source provides eight tuples.
  auto* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {6}, {1});
  auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();
  const std::vector<int32> featureIds = {0, 2, 1, 1, 0, 2};
  for(usize i = 0; i < featureIds.size(); i++)
  {
    featureIdsStoreRef[i] = featureIds[i];
  }

  auto* featureValuesPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {8}, {1});
  auto& featureValuesStoreRef = featureValuesPtr->getDataStoreRef();
  for(usize i = 0; i < featureValuesStoreRef.getNumberOfTuples(); i++)
  {
    featureValuesStoreRef[i] = static_cast<float32>(i) * 2.0F + 1.0F; // [1, 3, 5, 7, 9, 11, 13, 15]
  }

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // The hand-derived output is [1, 5, 3, 3, 1, 5].
  const DataPath createdPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(createdPath));
  const auto& createdRef = dataStructure.getDataRefAs<Float32Array>(createdPath).getDataStoreRef();
  const std::vector<float32> expected = {1.0F, 5.0F, 3.0F, 3.0F, 1.0F, 5.0F};
  for(usize i = 0; i < expected.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(createdRef[i] == expected[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Zero-tuple FeatureIds accepted", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // A zero-tuple FeatureIds array has nothing to copy but remains valid.
  // The filter must create empty outputs without entering range validation.
  DataStructure dataStructure;

  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {0}, {1});
  Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_FeatureTemperatureName, {4}, {1});

  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath createdPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(createdPath));
  const auto& createdRef = dataStructure.getDataRefAs<Float32Array>(createdPath).getDataStoreRef();
  REQUIRE(createdRef.getNumberOfTuples() == 0);
}

using ListOfTypes = std::tuple<int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64>;
TEMPLATE_LIST_TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Valid filter execution", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter]", ListOfTypes)
{
  UnitTest::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects Direct, Scanline, or both for the same assertions.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;

  // Create the cell-to-feature mapping.
  Int32Array* cellFeatureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {{10, 3}}, {1});
  REQUIRE(cellFeatureIdsPtr != nullptr);
  Int32Array& cellFeatureIds = *cellFeatureIdsPtr;

  for(usize y = 0; y < 3; y++)
  {
    for(usize x = 0; x < 10; x++)
    {
      usize index = (10 * y) + x;
      cellFeatureIds[index] = static_cast<int32>(y);
    }
  }

  // Distinct values in two source arrays expose an incorrect feature lookup.
  DataArray<TestType>* avgTempValuePtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureTemperatureName, {3}, {1});
  REQUIRE(avgTempValuePtr != nullptr);
  DataArray<TestType>& avgTempValue = *avgTempValuePtr;
  DataArray<TestType>* featureDataPtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureDataArrayName, {3}, {1});
  REQUIRE(featureDataPtr != nullptr);
  DataArray<TestType>& featureDataValue = *featureDataPtr;

  for(usize i = 0; i < 3; i++)
  {
    avgTempValue[i] = static_cast<TestType>(i * 10 + 5);    // [5, 15, 25]
    featureDataValue[i] = static_cast<TestType>(i * 3 + 1); // [1, 4, 7]
  }

  // Copy both source arrays through the selected algorithm path.
  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;

  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName}), DataPath({k_FeatureDataArrayName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Each output value must match its source feature tuple.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(k_CellTempArrayPath));
  const auto& createdElementTempArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellTempArrayPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(k_CellFeatureArrayPath));
  const auto& createdElementFeatureArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellFeatureArrayPath);
  REQUIRE(createdElementTempArray.getNumberOfTuples() == createdElementFeatureArray.getNumberOfTuples());
  for(usize i = 0; i < createdElementTempArray.getNumberOfTuples(); i++)
  {
    CAPTURE(i);
    int32 featureId = cellFeatureIds[i];
    TestType value1 = createdElementTempArray[i];
    TestType value2 = createdElementFeatureArray[i];
    TestType featureValue1 = avgTempValue[featureId];
    TestType featureValue2 = featureDataValue[featureId];
    REQUIRE(value1 == featureValue1);
    REQUIRE(value2 == featureValue2);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CopyFeatureArrayToElementArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CopyFeatureArrayToElementArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CopyFeatureArrayToElementArrayFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<CopyFeatureArrayToElementArrayFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::vector<DataPath>>(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPaths_Key) == std::vector<DataPath>{DataPath({"DataContainer", "CellData", "TestArray"})});
      CHECK(args.value<DataPath>(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Legacy CreatedArrayName does not map to the suffix parameter.
      // The copied array keeps its input name, so the suffix remains empty.
      CHECK(args.value<std::string>(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key).empty());
    }
  }
}
