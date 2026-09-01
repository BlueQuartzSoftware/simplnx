#include "SimplnxCore/Filters/Algorithms/MultiThresholdObjects.hpp"
#include "SimplnxCore/Filters/Algorithms/MultiThresholdObjectsDirect.hpp"
#include "SimplnxCore/Filters/Algorithms/MultiThresholdObjectsScanline.hpp"
#include "SimplnxCore/Filters/MultiThresholdObjectsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
const std::string k_TestArrayFloatName = "TestArrayFloat";
const std::string k_TestArrayIntName = "TestArrayInt";
const std::string k_ThresholdArrayName = "ThresholdArray";
const std::string k_MultiComponentArrayName = "MultiComponent";

const DataPath k_ImageCellDataName({k_ImageGeometry, k_CellData});
const DataPath k_TestArrayFloatPath = k_ImageCellDataName.createChildPath(k_TestArrayFloatName);
const DataPath k_TestArrayIntPath = k_ImageCellDataName.createChildPath(k_TestArrayIntName);
const DataPath k_MultiComponentArrayPath = k_ImageCellDataName.createChildPath(k_MultiComponentArrayName);
const DataPath k_ThresholdArrayPath = k_ImageCellDataName.createChildPath(k_ThresholdArrayName);

const DataPath k_MismatchingComponentsArrayPath = k_ImageCellDataName.createChildPath("MismatchingComponentsArray");
const DataPath k_MismatchingTuplesArrayPath({"MismatchingTuplesArray"});

/**
 * @class MultiThresholdFailingReadStore
 * @brief Injects a selected error into every bulk read.
 * @tparam T Specifies the store element type.
 */
template <typename T>
class MultiThresholdFailingReadStore : public DataStore<T>
{
public:
  /**
   * @brief Creates an in-memory store with a selected read error.
   * @param tupleShape Store tuple shape.
   * @param componentShape Store component shape.
   * @param initValue Optional initialization value.
   * @param errorCode Error code returned by bulk reads.
   */
  MultiThresholdFailingReadStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(m_ErrorCode, "Injected multi-threshold bulk-read failure");
  }

private:
  int32 m_ErrorCode = 0;
};

/**
 * @class MultiThresholdFailingWriteStore
 * @brief Injects a selected error into every bulk write.
 * @tparam T Specifies the store element type.
 */
template <typename T>
class MultiThresholdFailingWriteStore : public DataStore<T>
{
public:
  /**
   * @brief Creates an in-memory store with a selected write error.
   * @param tupleShape Store tuple shape.
   * @param componentShape Store component shape.
   * @param initValue Optional initialization value.
   * @param errorCode Error code returned by bulk writes.
   */
  MultiThresholdFailingWriteStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected multi-threshold bulk-write failure");
  }

private:
  int32 m_ErrorCode = 0;
};

/**
 * @class MultiThresholdCancelAfterReadStore
 * @brief Requests cancellation after the first successful bulk read.
 * @tparam T Specifies the store element type.
 */
template <typename T>
class MultiThresholdCancelAfterReadStore : public DataStore<T>
{
public:
  /**
   * @brief Creates an in-memory store that updates a caller-owned cancel flag.
   * @param tupleShape Store tuple shape.
   * @param componentShape Store component shape.
   * @param initValue Optional initialization value.
   * @param shouldCancel Cancel flag that must outlive this store.
   */
  MultiThresholdCancelAfterReadStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Performs a bulk read and requests cancellation if the read succeeds.
   * @param startIndex Zero-based first source element.
   * @param buffer Receives the selected values.
   * @return The underlying DataStore read result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Creates scalar and three-component arrays for threshold tests.
 * @return The populated 20-tuple DataStructure.
 */
DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  // The one-dimensional geometry supplies 20 tuples for predictable thresholds.
  ImageGeom* image = ImageGeom::Create(dataStructure, k_ImageGeometry);
  std::vector<usize> dims = {20, 1, 1};
  image->setDimensions(dims);

  ShapeType tDims = {20};
  ShapeType cDims = {1};
  ShapeType cDimsMulti = {3};
  float fnum = 0.0f;
  int inum = 0;
  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_CellData, tDims, image->getId());
  Float32Array* data = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_TestArrayFloatName, tDims, cDims, am->getId());
  Int32Array* data1 = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_TestArrayIntName, tDims, cDims, am->getId());
  Int32Array* multiComponentData = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_MultiComponentArrayName, tDims, cDimsMulti, am->getId());

  Float32Array* invalid1 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_MismatchingComponentsArrayPath.getTargetName(), tDims, cDimsMulti, am->getId());
  invalid1->fill(1.0);
  Float32Array* invalid2 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_MismatchingTuplesArrayPath.getTargetName(), std::vector<usize>{10}, cDims);
  invalid2->fill(2.0);

  usize numComponents = multiComponentData->getNumberOfComponents();
  int32 sign = 1;

  // The scalar arrays increase with the tuple index. The three-component array
  // alternates signs so component thresholds can select different tuples.
  for(usize i = 0; i < 20; i++)
  {
    fnum += 0.01f;
    (*data)[i] = fnum;
    (*data1)[i] = inum;
    multiComponentData->setComponent(i, 0, i * -sign);
    multiComponentData->setComponent(i, 1, i * sign);
    multiComponentData->setComponent(i, 2, i * -sign);
    sign *= -1;
    ++inum;
  }
  return dataStructure;
}

/**
 * @brief Produces a value below the representable range of T.
 * @tparam T Specifies the tested numeric type.
 * @return A lower out-of-range value represented as float64.
 */
template <typename T>
float64 GetOutOfBoundsMinimumValue()
{
  if constexpr(std::is_unsigned_v<T>)
  {
    return -1.0;
  }
  else if constexpr(std::is_floating_point_v<T>)
  {
    return static_cast<float64>(-std::numeric_limits<T>::max()) * 2;
  }

  return static_cast<float64>(std::numeric_limits<T>::min()) * 2;
}

/**
 * @brief Produces a value above the representable range of T.
 * @tparam T Specifies the tested numeric type.
 * @return An upper out-of-range value represented as float64.
 */
template <typename T>
float64 GetOutOfBoundsMaximumValue()
{
  return static_cast<float64>(std::numeric_limits<T>::max()) * 2;
}
} // namespace

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateTestDataStructure();

  SECTION("Float Array Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* thresholdArray = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
    REQUIRE(thresholdArray != nullptr);

    // A 0.1 threshold rejects indices 0 through 9 and selects indices 10 through 19.
    for(usize i = 0; i < 20; i++)
    {
      if(i < 10)
      {
        REQUIRE((*thresholdArray)[i] == false);
      }
      else
      {
        REQUIRE((*thresholdArray)[i] == true);
      }
    }
  }

  SECTION("Int Array Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayIntPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(15);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* thresholdArray = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
    REQUIRE(thresholdArray != nullptr);

    // A 0.1 threshold rejects indices 0 through 9 and selects indices 10 through 19.
    for(usize i = 0; i < 20; i++)
    {
      if(i <= 15)
      {
        REQUIRE((*thresholdArray)[i] == false);
      }
      else
      {
        REQUIRE((*thresholdArray)[i] == true);
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Top-level inversion is logical", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateTestDataStructure();
  MultiThresholdObjectsFilter filter;
  Arguments args;

  ArrayThresholdSet thresholdSet;
  thresholdSet.setInverted(true);
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(10);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto& thresholdArray = dataStructure.getDataRefAs<BoolArray>(k_ThresholdArrayPath);
  for(usize i = 0; i < 20; ++i)
  {
    REQUIRE(thresholdArray[i] == (i <= 10));
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Nested sets compose before root inversion", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateTestDataStructure();
  auto greaterThan = [](int32 value) {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayIntPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(value);
    return threshold;
  };
  auto lessThan = [](int32 value) {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayIntPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::LessThan);
    threshold->setComparisonValue(value);
    return threshold;
  };

  auto innerHigh = greaterThan(10);
  auto innerLow = lessThan(5);
  innerLow->setUnionOperator(IArrayThreshold::UnionOperator::Or);
  auto nestedSet = std::make_shared<ArrayThresholdSet>();
  nestedSet->setArrayThresholds({innerHigh, innerLow});
  nestedSet->setUnionOperator(IArrayThreshold::UnionOperator::And);

  ArrayThresholdSet thresholdSet;
  thresholdSet.setInverted(true);
  thresholdSet.setArrayThresholds({greaterThan(2), nestedSet});

  MultiThresholdObjectsFilter filter;
  Arguments args;
  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto& output = dataStructure.getDataRefAs<BoolArray>(k_ThresholdArrayPath);
  for(usize i = 0; i < 20; ++i)
  {
    const bool beforeInversion = i > 2 && (i > 10 || i < 5);
    REQUIRE(output[i] == !beforeInversion);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Scanline propagates bulk I/O failures", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  constexpr int32 k_ReadError = -73001;
  constexpr int32 k_WriteError = -73002;

  auto makeValues = [](DataStructure& dataStructure, const DataPath& inputPath, const DataPath& outputPath) {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(inputPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0);
    ArrayThresholdSet thresholdSet;
    thresholdSet.setArrayThresholds({threshold});
    MultiThresholdObjectsInputValues values;
    values.ArrayThresholdsObject = thresholdSet;
    values.CreatedMaskType = DataType::boolean;
    values.OutputDataArrayName = outputPath.getTargetName();
    values.UseCustomTrueValue = false;
    values.UseCustomFalseValue = false;
    return values;
  };

  SECTION("input bulk read")
  {
    DataStructure dataStructure = CreateTestDataStructure();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
    const DataPath inputPath = k_ImageCellDataName.createChildPath("Failing Input");
    const DataPath outputPath = k_ImageCellDataName.createChildPath("Failing Mask");
    auto inputStore = std::make_shared<MultiThresholdFailingReadStore<int32>>(cellData.getShape(), ShapeType{1}, int32{1}, k_ReadError);
    auto outputStore = std::make_shared<BoolDataStore>(cellData.getShape(), ShapeType{1}, false);
    REQUIRE(Int32Array::Create(dataStructure, inputPath.getTargetName(), inputStore, cellData.getId()) != nullptr);
    REQUIRE(BoolArray::Create(dataStructure, outputPath.getTargetName(), outputStore, cellData.getId()) != nullptr);
    auto values = makeValues(dataStructure, inputPath, outputPath);
    std::atomic_bool shouldCancel = false;
    Result<> result = MultiThresholdObjectsScanline(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_ReadError);
  }

  SECTION("output bulk write")
  {
    DataStructure dataStructure = CreateTestDataStructure();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
    const DataPath inputPath = k_ImageCellDataName.createChildPath("Writable Input");
    const DataPath outputPath = k_ImageCellDataName.createChildPath("Failing Mask");
    auto inputStore = std::make_shared<Int32DataStore>(cellData.getShape(), ShapeType{1}, int32{1});
    auto outputStore = std::make_shared<MultiThresholdFailingWriteStore<bool>>(cellData.getShape(), ShapeType{1}, false, k_WriteError);
    REQUIRE(Int32Array::Create(dataStructure, inputPath.getTargetName(), inputStore, cellData.getId()) != nullptr);
    REQUIRE(BoolArray::Create(dataStructure, outputPath.getTargetName(), outputStore, cellData.getId()) != nullptr);
    auto values = makeValues(dataStructure, inputPath, outputPath);
    std::atomic_bool shouldCancel = false;
    Result<> result = MultiThresholdObjectsScanline(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Direct and Scanline evaluate the same nested tree", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  auto makeThreshold = [](const DataPath& path, usize component, ArrayThreshold::ComparisonType comparison, float64 value, IArrayThreshold::UnionOperator unionOperator) {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(path);
    threshold->setComponentIndex(component);
    threshold->setComparisonType(comparison);
    threshold->setComparisonValue(value);
    threshold->setUnionOperator(unionOperator);
    return threshold;
  };
  ArrayThresholdSet nestedSet;
  nestedSet.setInverted(true);
  nestedSet.setArrayThresholds({makeThreshold(k_TestArrayIntPath, 0, ArrayThreshold::ComparisonType::GreaterThan, 2, IArrayThreshold::UnionOperator::And),
                                makeThreshold(k_TestArrayIntPath, 0, ArrayThreshold::ComparisonType::LessThan, 18, IArrayThreshold::UnionOperator::And)});
  auto nested = std::make_shared<ArrayThresholdSet>(nestedSet);
  nested->setUnionOperator(IArrayThreshold::UnionOperator::And);
  ArrayThresholdSet thresholdSet;
  thresholdSet.setInverted(true);
  thresholdSet.setArrayThresholds({nested, makeThreshold(k_MultiComponentArrayPath, 1, ArrayThreshold::ComparisonType::GreaterThan, 5, IArrayThreshold::UnionOperator::Or),
                                   makeThreshold(k_TestArrayIntPath, 0, ArrayThreshold::ComparisonType::Operator_NotEqual, 7, IArrayThreshold::UnionOperator::And)});

  DataStructure directDataStructure = CreateTestDataStructure();
  DataStructure scanlineDataStructure = CreateTestDataStructure();
  const auto& directCellData = directDataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
  const auto& scanlineCellData = scanlineDataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
  const DataPath directOutputPath = k_ImageCellDataName.createChildPath("Direct Custom Mask");
  const DataPath scanlineOutputPath = k_ImageCellDataName.createChildPath("Scanline Custom Mask");
  REQUIRE(UInt8Array::CreateWithStore<UInt8DataStore>(directDataStructure, directOutputPath.getTargetName(), directCellData.getShape(), ShapeType{1}, directCellData.getId()) != nullptr);
  REQUIRE(UInt8Array::CreateWithStore<UInt8DataStore>(scanlineDataStructure, scanlineOutputPath.getTargetName(), scanlineCellData.getShape(), ShapeType{1}, scanlineCellData.getId()) != nullptr);
  MultiThresholdObjectsInputValues directValues;
  directValues.ArrayThresholdsObject = thresholdSet;
  directValues.CreatedMaskType = DataType::uint8;
  directValues.OutputDataArrayName = directOutputPath.getTargetName();
  directValues.UseCustomTrueValue = true;
  directValues.CustomTrueValue = 19;
  directValues.UseCustomFalseValue = true;
  directValues.CustomFalseValue = 4;
  MultiThresholdObjectsInputValues scanlineValues = directValues;
  scanlineValues.OutputDataArrayName = scanlineOutputPath.getTargetName();
  std::atomic_bool directCancel = false;
  std::atomic_bool scanlineCancel = false;
  SIMPLNX_RESULT_REQUIRE_VALID(MultiThresholdObjectsDirect(directDataStructure, IFilter::MessageHandler{}, directCancel, &directValues)())
  SIMPLNX_RESULT_REQUIRE_VALID(MultiThresholdObjectsScanline(scanlineDataStructure, IFilter::MessageHandler{}, scanlineCancel, &scanlineValues)())
  const auto& directOutput = directDataStructure.getDataRefAs<UInt8Array>(directOutputPath);
  const auto& scanlineOutput = scanlineDataStructure.getDataRefAs<UInt8Array>(scanlineOutputPath);
  REQUIRE(directOutput.getNumberOfTuples() == scanlineOutput.getNumberOfTuples());
  for(usize i = 0; i < directOutput.getNumberOfTuples(); ++i)
  {
    REQUIRE(directOutput[i] == scanlineOutput[i]);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Direct and Scanline honor cancellation without completing output", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  auto makeValues = [](const DataPath& inputPath, const DataPath& outputPath) {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(inputPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0);
    ArrayThresholdSet thresholdSet;
    thresholdSet.setArrayThresholds({threshold});
    MultiThresholdObjectsInputValues values;
    values.ArrayThresholdsObject = thresholdSet;
    values.CreatedMaskType = DataType::boolean;
    values.OutputDataArrayName = outputPath.getTargetName();
    values.UseCustomTrueValue = false;
    values.UseCustomFalseValue = false;
    return values;
  };

  SECTION("Direct")
  {
    DataStructure dataStructure = CreateTestDataStructure();
    const DataPath outputPath = k_ImageCellDataName.createChildPath("Cancelled Direct Mask");
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
    auto* output = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, outputPath.getTargetName(), cellData.getShape(), ShapeType{1}, cellData.getId());
    REQUIRE(output != nullptr);
    output->fill(true);
    auto values = makeValues(k_TestArrayIntPath, outputPath);
    std::atomic_bool shouldCancel = true;
    SIMPLNX_RESULT_REQUIRE_VALID(MultiThresholdObjectsDirect(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)())
    for(usize i = 0; i < output->getNumberOfTuples(); ++i)
    {
      REQUIRE(output->getValue(i));
    }
  }

  SECTION("Scanline")
  {
    DataStructure dataStructure = CreateTestDataStructure();
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_ImageCellDataName);
    const DataPath inputPath = k_ImageCellDataName.createChildPath("Cancel Input");
    const DataPath outputPath = k_ImageCellDataName.createChildPath("Cancelled Scanline Mask");
    std::atomic_bool shouldCancel = false;
    auto inputStore = std::make_shared<MultiThresholdCancelAfterReadStore<int32>>(cellData.getShape(), ShapeType{1}, int32{1}, shouldCancel);
    auto* input = Int32Array::Create(dataStructure, inputPath.getTargetName(), inputStore, cellData.getId());
    auto* output = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, outputPath.getTargetName(), cellData.getShape(), ShapeType{1}, cellData.getId());
    REQUIRE(input != nullptr);
    REQUIRE(output != nullptr);
    output->fill(true);
    auto values = makeValues(inputPath, outputPath);
    SIMPLNX_RESULT_REQUIRE_VALID(MultiThresholdObjectsScanline(dataStructure, IFilter::MessageHandler{}, shouldCancel, &values)())
    REQUIRE(shouldCancel);
    for(usize i = 0; i < output->getNumberOfTuples(); ++i)
    {
      REQUIRE(output->getValue(i));
    }
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution - Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64,
                   float32, float64)
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue = 25;
  float64 falseValue = 10;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* thresholdArray = dataStructure.getDataAs<DataArray<TestType>>(k_ThresholdArrayPath);
  REQUIRE(thresholdArray != nullptr);

  // A 0.1 threshold rejects indices 0 through 9 and selects indices 10 through 19.
  for(usize i = 0; i < 20; i++)
  {
    if(i <= 15)
    {
      REQUIRE((*thresholdArray)[i] == falseValue);
    }
    else
    {
      REQUIRE((*thresholdArray)[i] == trueValue);
    }
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));

  SECTION("Empty ArrayThresholdSet")
  {
    ArrayThresholdSet thresholdSet;

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Empty ArrayThreshold DataPath")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Out of Bounds Component Index")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    threshold->setComponentIndex(1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Mismatching Tuples in Threshold Arrays")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold1 = std::make_shared<ArrayThreshold>();
    threshold1->setArrayPath(k_TestArrayFloatPath);
    threshold1->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold1->setComparisonValue(0.1);
    auto threshold2 = std::make_shared<ArrayThreshold>();
    threshold2->setArrayPath(k_MismatchingTuplesArrayPath);
    threshold2->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold2->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold1, threshold2});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Out of Bounds Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64, float32)
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue;
  float64 falseValue;
  int32 code;

  SECTION("True Value < Minimum")
  {
    trueValue = GetOutOfBoundsMinimumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value < Minimum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMinimumValue<TestType>();
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseOutOfBounds;
  }

  SECTION("True Value > Maximum")
  {
    trueValue = GetOutOfBoundsMaximumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value > Maximum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMaximumValue<TestType>();
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseOutOfBounds;
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Boolean Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  int32 code;

  SECTION("Custom True Value")
  {
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueWithBoolean;
    args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  }

  SECTION("Custom False Value")
  {
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseWithBoolean;
    args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Requires a threshold mask to select tuple indices 10 through 19.
 * @tparam T Specifies the Boolean or uint8 mask type.
 * @param dataStructure Contains the threshold mask.
 * @param thresholdArrayPath Threshold mask path.
 */
template <typename T>
void checkMaskValues(const DataStructure& dataStructure, const DataPath& thresholdArrayPath)
{
  auto* thresholdArrayPtr = dataStructure.getDataAs<DataArray<T>>(thresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdArray = (*thresholdArrayPtr);

  // A 0.1 threshold rejects indices 0 through 9 and selects indices 10 through 19.
  for(usize i = 0; i < 20; i++)
  {
    if(i < 10)
    {
      REQUIRE(thresholdArray[i] == static_cast<T>(0));
    }
    else
    {
      REQUIRE(thresholdArray[i] == static_cast<T>(1));
    }
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution, DataType", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateTestDataStructure();

  // Verify each supported signed integer output type.
  SECTION("Int8 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int8));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int8>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int16 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int16));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int16>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int32 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int32));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int64 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int64));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int64>(dataStructure, k_ThresholdArrayPath);
  }

  // Verify each supported unsigned integer output type.
  SECTION("UInt8 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint8));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint8>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt16 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint16));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint16>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt32 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint32));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt64 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint64));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint64>(dataStructure, k_ThresholdArrayPath);
  }

  // Verify each supported floating-point output type.
  SECTION("Float32 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float32));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<float32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Float64 Threshold")
  {
    MultiThresholdObjectsFilter filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float64));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<float64>(dataStructure, k_ThresholdArrayPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution - Multicomponent", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateTestDataStructure();

  MultiThresholdObjectsFilter filter;
  Arguments args;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_MultiComponentArrayPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(0);
  threshold->setComponentIndex(1);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* thresholdArray = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArray != nullptr);

  usize numTuples = thresholdArray->getNumberOfTuples();

  // The threshold selects positive Y components at even tuple indices, except index 0.
  REQUIRE_FALSE((*thresholdArray)[0]);
  for(usize i = 1; i < numTuples; i++)
  {
    bool value = (*thresholdArray)[i];
    if(i % 2 == 0)
    {
      REQUIRE(value);
    }
    else
    {
      REQUIRE_FALSE(value);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjectsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][MultiThresholdObjectsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MultiThresholdObjectsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "MultiThresholdObjectsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<MultiThresholdObjectsFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(MultiThresholdObjectsFilter::k_CreatedDataName_Key) == "TestName");
    }
  }
}
