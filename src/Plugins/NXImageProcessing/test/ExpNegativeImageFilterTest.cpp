#include "Float64MathConsumerTest.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/BinaryThresholdImageFilter.hpp"
#include "NXImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "NXImageProcessing/Filters/ExpImageFilter.hpp"
#include "NXImageProcessing/Filters/ExpNegativeImageFilter.hpp"
#include "NXImageProcessing/Filters/Log10ImageFilter.hpp"
#include "NXImageProcessing/Filters/LogImageFilter.hpp"
#include "NXImageProcessing/Filters/SqrtImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros expand to these constructs.
TEMPLATE_TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: integer input produces float64 decay", "[NXImageProcessing][ExpNegativeImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{0, 1, std::numeric_limits<TestType>::max(), -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{0, 1, std::numeric_limits<TestType>::max()};
    }
  }();
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = ip_test::RunMappedPointwiseFilter<ExpNegativeImageFilter>(dataStructure, input, DataType::float64, {1, 1, values.size()}, {1});
  const Float64ArrayType* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(exponentialPtr->getDataType() == DataType::float64);
  REQUIRE(exponentialPtr->getTupleShape() == ShapeType{1, 1, values.size()});
  REQUIRE(exponentialPtr->getComponentShape() == ShapeType{1});
  REQUIRE(exponentialPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = exponentialPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 1.0);
  REQUIRE(store.getValue(1) == Approx(0.36787944117144232160).epsilon(1.0e-15));
  if constexpr(std::is_same_v<TestType, int8>)
  {
    REQUIRE(store.getValue(2) > 0.0);
    REQUIRE(store.getValue(2) == Approx(6.99198999664591705080e-56).epsilon(1.0e-15));
  }
  else if constexpr(std::is_same_v<TestType, uint8>)
  {
    REQUIRE(store.getValue(2) > 0.0);
    REQUIRE(store.getValue(2) == Approx(1.79848622027946356313e-111).epsilon(1.0e-15));
  }
  else
  {
    REQUIRE(store.getValue(2) == 0.0);
    REQUIRE_FALSE(std::signbit(store.getValue(2)));
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(3) == Approx(2.71828182845904523536).epsilon(1.0e-15));
    if constexpr(std::is_same_v<TestType, int8>)
    {
      REQUIRE(store.getValue(4) == Approx(3.88770840599459509222e55).epsilon(1.0e-15));
    }
    else
    {
      REQUIRE(store.getValue(4) == std::numeric_limits<float64>::infinity());
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: floating IEEE values retain sign reversed range behavior", "[NXImageProcessing][ExpNegativeImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::quiet_NaN(),
                                                                                   std::numeric_limits<TestType>::infinity(),
                                                                                   -std::numeric_limits<TestType>::infinity(),
                                                                                   TestType{0},
                                                                                   -TestType{0},
                                                                                   -100,
                                                                                   128,
                                                                                   -1000,
                                                                                   1000,
                                                                                   100,
                                                                                   103,
                                                                                   104,
                                                                                   -10,
                                                                                   -88,
                                                                                   -89,
                                                                                   -709,
                                                                                   -710,
                                                                                   744,
                                                                                   745,
                                                                                   746,
                                                                                   -1,
                                                                                   1});
  const DataPath output = ip_test::RunMappedPointwiseFilter<ExpNegativeImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 22}, {1});
  const DataArray<TestType>* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(exponentialPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(exponentialPtr->getTupleShape() == ShapeType{1, 1, 22});
  REQUIRE(exponentialPtr->getComponentShape() == ShapeType{1});
  REQUIRE(exponentialPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = exponentialPtr->getDataStoreRef();
  REQUIRE(std::isnan(store.getValue(0)));
  REQUIRE(store.getValue(1) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(1)));
  REQUIRE(store.getValue(2) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(3) == 1);
  REQUIRE(store.getValue(4) == 1);
  REQUIRE(store.getValue(7) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(8) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(8)));
  constexpr float64 relativeTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(store.getValue(20) == Approx(2.71828182845904523536).epsilon(relativeTolerance));
  REQUIRE(store.getValue(21) == Approx(0.36787944117144232160).epsilon(relativeTolerance));
  REQUIRE(store.getValue(16) == std::numeric_limits<TestType>::infinity());
  if constexpr(std::is_same_v<TestType, float32>)
  {
    REQUIRE(store.getValue(5) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(6) == 0);
    REQUIRE(store.getValue(9) == 3.7835058536770060915e-44f);
    REQUIRE(store.getValue(10) == std::numeric_limits<float32>::denorm_min());
    REQUIRE(store.getValue(11) == 0);
    REQUIRE(store.getValue(12) == Approx(22026.46579480671651696).epsilon(1.0e-7));
    REQUIRE(store.getValue(13) == Approx(1.65163625499400185553e38).epsilon(1.0e-7));
    REQUIRE(store.getValue(14) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(15) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(17) == 0);
    REQUIRE(store.getValue(18) == 0);
    REQUIRE(store.getValue(19) == 0);
  }
  else
  {
    REQUIRE(store.getValue(5) == Approx(2.68811714181613544841e43).epsilon(1.0e-15));
    REQUIRE(store.getValue(6) > 0.0);
    REQUIRE(store.getValue(6) == Approx(2.57220937264241482684e-56).epsilon(1.0e-15));
    REQUIRE(store.getValue(9) > 0.0);
    REQUIRE(store.getValue(9) == Approx(3.72007597602083596296e-44).epsilon(1.0e-15));
    REQUIRE(store.getValue(10) > 0.0);
    REQUIRE(store.getValue(10) == Approx(1.85211676951797542963e-45).epsilon(1.0e-15));
    REQUIRE(store.getValue(11) > 0.0);
    REQUIRE(store.getValue(11) == Approx(6.81355682154529853676e-46).epsilon(1.0e-15));
    REQUIRE(store.getValue(12) == Approx(22026.46579480671651696).epsilon(1.0e-15));
    REQUIRE(store.getValue(13) == Approx(1.65163625499400185553e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(14) == Approx(4.48961281917434524628e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(15) == Approx(8.21840746155497218924e307).epsilon(1.0e-15));
    // exp(-744) < 2d, exp(-745) < d, and exp(-746) < d/2, where d = 2^-1074.
    // Accept zero or bounded subnormal results without requiring a particular libm rounding.
    for(usize index = 17; index < 20; ++index)
    {
      const float64 value = store.getValue(index);
      const float64 upperBound = index == 17 ? 2 * std::numeric_limits<float64>::denorm_min() : std::numeric_limits<float64>::denorm_min();
      CAPTURE(index);
      REQUIRE(std::isfinite(value));
      REQUIRE(value >= 0);
      REQUIRE_FALSE(std::signbit(value));
      REQUIRE(value <= upperBound);
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: promoted output preserves tuples and components", "[NXImageProcessing][ExpNegativeImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto input = ip_test::BuildScalarValuesImage<int16>(dataStructure, {0, 0});
  Int16ArrayType* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16ArrayType>(input));
  const ShapeType tupleShape{1, 1, 2};
  const ShapeType componentShape{2};
  auto inputStore = data_store_utilities::CreateDataStore<int16>(dataStructure, input, tupleShape, componentShape);
  REQUIRE(inputStore != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> values = {-1, 0, 1, 2};
  const auto writeResult = inputStore->copyFromBuffer(0, nonstd::span<const int16>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const auto output = ip_test::RunMappedPointwiseFilter<ExpNegativeImageFilter>(dataStructure, input, DataType::float64, tupleShape, componentShape);
  const Float64ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == Approx(2.71828182845904523536).epsilon(1.0e-15));
  REQUIRE(store.getValue(1) == 1.0);
  REQUIRE(store.getValue(2) == Approx(0.36787944117144232160).epsilon(1.0e-15));
  REQUIRE(store.getValue(3) == Approx(0.13533528323661269189).epsilon(1.0e-15));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// exp(-(-1000)) exceeds the Float64 range and produces positive infinity.
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: exp(-x) of a large negative input overflows to +inf (domain edge)", "[NXImageProcessing][ExpNegativeImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {-1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpNegativeImageFilter>(dataStructure, input);

  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // exp(-(-1000)) = exp(1000) = +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpNegativeImageTest.cpp(defaults):
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR filter (defaults), then compare with the
// committed baseline @0.01. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ExpNegativeImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  ExpNegativeImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_ExpNegativeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros expand to these constructs.
namespace
{
void executeMathTestStep(const IFilter& filter, DataStructure& dataStructure, const Arguments& args, const DataPath& output, DataType type, const ShapeType& tupleShape)
{
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  REQUIRE(preflight.output_actions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflight.output_actions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->path() == output);
  REQUIRE(actionPtr->type() == type);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

template <class T>
void checkMathTestArray(const DataStructure& dataStructure, const DataPath& path, const std::vector<T>& expected, IDataStore::StoreType expectedStore, float64 relativeTolerance = 0.0)
{
  CAPTURE(path.toString());
  const auto* arrayPtr = dataStructure.getDataAs<DataArray<T>>(path);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getTupleShape() == ShapeType{1, 1, expected.size()});
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  REQUIRE(arrayPtr->getStoreType() == expectedStore);
  if(expectedStore == IDataStore::StoreType::OutOfCore)
  {
    unit_test::RequireDiskBackedStore(*arrayPtr);
  }
  std::vector<T> values(expected.size());
  const auto readResult = arrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    const auto actual = values[index];
    const auto target = expected[index];
    if constexpr(std::is_floating_point_v<T>)
    {
      if(std::isnan(target))
      {
        REQUIRE(std::isnan(actual));
        continue;
      }
      if(std::isfinite(target) && target != 0 && relativeTolerance != 0)
      {
        REQUIRE(actual == Approx(target).epsilon(relativeTolerance));
        continue;
      }
      REQUIRE(std::signbit(actual) == std::signbit(target));
    }
    REQUIRE(actual == target);
  }
}

struct MathConsumerExpectations
{
  std::vector<float64> Quantitative;
  float64 LowerThreshold;
  float64 UpperThreshold;
  std::vector<uint8> Mask;
  std::vector<uint32> Labels;
};

void checkThresholdAndLabels(DataStructure& dataStructure, const DataPath& quantitative, const MathConsumerExpectations& expected, IDataStore::StoreType expectedStore)
{
  const ShapeType tupleShape{1, 1, expected.Quantitative.size()};
  const auto mask = quantitative.replaceName("Selected");
  auto thresholdArgs = ip_test::PointwiseFilterArguments<BinaryThresholdImageFilter>(quantitative);
  thresholdArgs.insertOrAssign(BinaryThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Selected"));
  thresholdArgs.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(expected.LowerThreshold));
  thresholdArgs.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(expected.UpperThreshold));
  thresholdArgs.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(1));
  thresholdArgs.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(0));

  checkMathTestArray(dataStructure, quantitative, expected.Quantitative, expectedStore, 1.0e-15);
  executeMathTestStep(BinaryThresholdImageFilter{}, dataStructure, thresholdArgs, mask, DataType::uint8, tupleShape);
  checkMathTestArray(dataStructure, quantitative, expected.Quantitative, expectedStore, 1.0e-15);
  checkMathTestArray(dataStructure, mask, expected.Mask, expectedStore);

  const auto labels = mask.replaceName("Components");
  auto componentArgs = ip_test::PointwiseFilterArguments<ConnectedComponentImageFilter>(mask);
  componentArgs.insertOrAssign(ConnectedComponentImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Components"));
  componentArgs.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  executeMathTestStep(ConnectedComponentImageFilter{}, dataStructure, componentArgs, labels, DataType::uint32, tupleShape);
  checkMathTestArray(dataStructure, labels, expected.Labels, expectedStore);
  checkMathTestArray(dataStructure, mask, expected.Mask, expectedStore);
  checkMathTestArray(dataStructure, quantitative, expected.Quantitative, expectedStore, 1.0e-15);
}

template <class Filter, class T>
void checkMathConsumerCase(const std::vector<T>& source, const MathConsumerExpectations& expected, IDataStore::StoreType expectedStore)
{
  DataStructure dataStructure;
  const auto input = ip_test::BuildScalarValuesImage(dataStructure, source);
  const auto quantitative = input.replaceName("Quantitative");
  auto mathArgs = ip_test::PointwiseFilterArguments<Filter>(input);
  mathArgs.insertOrAssign(Filter::k_OutputImageArrayName_Key, std::make_any<std::string>("Quantitative"));
  checkMathTestArray(dataStructure, input, source, expectedStore);
  executeMathTestStep(Filter{}, dataStructure, mathArgs, quantitative, DataType::float64, {1, 1, source.size()});
  checkMathTestArray(dataStructure, input, source, expectedStore);
  checkThresholdAndLabels(dataStructure, quantitative, expected, expectedStore);
  checkMathTestArray(dataStructure, input, source, expectedStore);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

void nx::core::UnitTest::CheckFloat64MathConsumers(IDataStore::StoreType expectedStore)
{
  constexpr float64 nan = std::numeric_limits<float64>::quiet_NaN();
  constexpr float64 inf = std::numeric_limits<float64>::infinity();
  SECTION("Exponential growth")
  {
    checkMathConsumerCase<ExpImageFilter, int16>({0, 1}, {.Quantitative = {1, std::numbers::e}, .LowerThreshold = 1, .UpperThreshold = 2, .Mask = {1, 0}, .Labels = {1, 0}}, expectedStore);
  }
  SECTION("Exponential decay retains values between zero and one")
  {
    checkMathConsumerCase<ExpNegativeImageFilter, int16>(
        {0, 1, 2}, {.Quantitative = {1, 0.3678794411714423, 0.1353352832366127}, .LowerThreshold = 0.1, .UpperThreshold = 0.5, .Mask = {0, 1, 1}, .Labels = {0, 1, 1}}, expectedStore);
  }
  SECTION("Natural logarithm")
  {
    checkMathConsumerCase<LogImageFilter, int16>(
        {1, 2, 0, -1}, {.Quantitative = {0, std::numbers::ln2, -inf, nan}, .LowerThreshold = 0.5, .UpperThreshold = 1, .Mask = {0, 1, 0, 0}, .Labels = {0, 1, 0, 0}}, expectedStore);
  }
  SECTION("Base-ten logarithm")
  {
    checkMathConsumerCase<Log10ImageFilter, int16>(
        {1, 2, 10, 0, -1}, {.Quantitative = {0, 0.3010299956639812, 1, -inf, nan}, .LowerThreshold = 0.25, .UpperThreshold = 0.5, .Mask = {0, 1, 0, 0, 0}, .Labels = {0, 1, 0, 0, 0}}, expectedStore);
  }
  SECTION("Square root")
  {
    checkMathConsumerCase<SqrtImageFilter, int16>(
        {0, 2, 4, -1}, {.Quantitative = {0, std::numbers::sqrt2, 2, nan}, .LowerThreshold = 1.4, .UpperThreshold = 1.5, .Mask = {0, 1, 0, 0}, .Labels = {0, 1, 0, 0}}, expectedStore);
  }
  SECTION("Finite exponential results form two disconnected regions")
  {
    checkMathConsumerCase<ExpImageFilter, float64>(
        {0, 1000, nan, -inf, inf}, {.Quantitative = {1, inf, nan, 0, inf}, .LowerThreshold = 0, .UpperThreshold = 2, .Mask = {1, 0, 0, 1, 0}, .Labels = {1, 0, 0, 2, 0}}, expectedStore);
  }
  SECTION("Nonfinite logarithms fall outside finite thresholds")
  {
    checkMathConsumerCase<LogImageFilter, float64>(
        {1, 0, -1, inf, nan}, {.Quantitative = {0, -inf, nan, inf, nan}, .LowerThreshold = -1, .UpperThreshold = 1, .Mask = {1, 0, 0, 0, 0}, .Labels = {1, 0, 0, 0, 0}}, expectedStore);
  }
  SECTION("Base-ten logarithm includes both ends of the interval")
  {
    checkMathConsumerCase<Log10ImageFilter, float64>({1, 10, 0, -1}, {.Quantitative = {0, 1, -inf, nan}, .LowerThreshold = 0, .UpperThreshold = 1, .Mask = {1, 1, 0, 0}, .Labels = {1, 1, 0, 0}},
                                                     expectedStore);
  }
  SECTION("Finite square roots")
  {
    checkMathConsumerCase<SqrtImageFilter, float64>(
        {0, 4, -1, inf, nan}, {.Quantitative = {0, 2, nan, inf, nan}, .LowerThreshold = 0, .UpperThreshold = 2, .Mask = {1, 1, 0, 0, 0}, .Labels = {1, 1, 0, 0, 0}}, expectedStore);
  }
  SECTION("Floating exponential decay")
  {
    checkMathConsumerCase<ExpNegativeImageFilter, float64>(
        {0, 1, 2}, {.Quantitative = {1, 0.3678794411714423, 0.1353352832366127}, .LowerThreshold = 0.5, .UpperThreshold = 1, .Mask = {1, 0, 0}, .Labels = {1, 0, 0}}, expectedStore);
  }
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: quantitative math outputs feed resident thresholds and component labels", "[NXImageProcessing][ExpNegativeImageFilter][ImageFilterIntegration]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  UnitTest::CheckFloat64MathConsumers(IDataStore::StoreType::InMemory);
}
