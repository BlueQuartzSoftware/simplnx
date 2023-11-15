#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ArrayCalculatorFilter.hpp"
#include "ComplexCore/utils/CalculatorItem.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IParameter.hpp"
#include "complex/Parameters/CalculatorParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

using namespace complex;

namespace
{
const std::string k_AttributeMatrix = "AttributeMatrix";
const std::string k_NumericMatrix = "NumericMatrix";
const std::string k_InputArray1 = "InputArray1";
const std::string k_InputArray2 = "InputArray2";
const std::string k_SpacedArray = "Spaced Array";
const std::string k_MultiComponentArray1 = "MultiComponent Array1";
const std::string k_MultiComponentArray2 = "MultiComponent Array2";
const std::string k_NumberArray = "4";
const std::string k_SignArray = "*";
const std::string k_CalculatedArray = "NewArray";
const DataPath k_AttributeMatrixPath = DataPath({k_AttributeMatrix});
const DataPath k_NumericMatrixPath = DataPath({k_NumericMatrix});
const DataPath k_InputArray1Path = k_AttributeMatrixPath.createChildPath(k_InputArray1);
const DataPath k_InputArray2Path = k_AttributeMatrixPath.createChildPath(k_InputArray2);
const DataPath k_SpacedArrayPath = k_AttributeMatrixPath.createChildPath(k_SpacedArray);
const DataPath k_MultiComponentArray1Path = k_AttributeMatrixPath.createChildPath(k_MultiComponentArray1);
const DataPath k_MultiComponentArray2Path = k_AttributeMatrixPath.createChildPath(k_MultiComponentArray2);
const DataPath k_NumberArrayPath = k_AttributeMatrixPath.createChildPath(k_NumberArray);
const DataPath k_SignArrayPath = k_AttributeMatrixPath.createChildPath(k_SignArray);

const DataPath k_NumericArrayPath({k_NumericMatrix, k_CalculatedArray});
const DataPath k_AttributeArrayPath({k_AttributeMatrix, k_CalculatedArray});

const std::string k_Pi_Str = StringUtilities::number(numbers::pi);

// -----------------------------------------------------------------------------
DataStructure createDataStructure()
{
  DataStructure dataStructure;
  AttributeMatrix* am1 = AttributeMatrix::Create(dataStructure, k_AttributeMatrix, {10ULL});
  const auto am1Id = am1->getId();
  AttributeMatrix* am2 = AttributeMatrix::Create(dataStructure, k_NumericMatrix, {1ULL});
  Float32Array* array1 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_InputArray1, {10}, {1}, am1Id);
  array1->fill(-12);
  UInt32Array* array2 = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_InputArray2, {10}, {1}, am1Id);
  array2->fill(10);
  UInt32Array* sArray = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_SpacedArray, {10}, {1}, am1Id);
  sArray->fill(2);

  UInt32Array* mcArray1 = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_MultiComponentArray1, std::vector<size_t>(1, 10), std::vector<size_t>(1, 3), am1Id);
  int num = 0;
  for(int i = 0; i < mcArray1->getNumberOfTuples() * mcArray1->getNumberOfComponents(); i++)
  {
    (*mcArray1)[i] = num;
    num++;
  }

  UInt32Array* mcArray2 = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_MultiComponentArray2, std::vector<size_t>(1, 10), std::vector<size_t>(1, 3), am1Id);
  num = 0;
  for(int i = 0; i < mcArray2->getNumberOfTuples() * mcArray2->getNumberOfComponents(); i++)
  {
    (*mcArray2)[i] = num;
    num++;
  }

  auto numberArray = mcArray2->deepCopy(k_NumberArrayPath);
  auto signArray = mcArray1->deepCopy(k_SignArrayPath);

  return dataStructure;
}
} // namespace

// -----------------------------------------------------------------------------
IFilter::ExecuteResult createAndExecuteArrayCalculatorFilter(const std::string& equation, const DataPath& calculatedPath, const CalculatorParameter::AngleUnits& units, DataStructure& dataStructure,
                                                             ArrayCalculatorFilter& filter)
{
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath({k_AttributeMatrix}), equation, units}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(calculatedPath));

  return filter.execute(dataStructure, args);
}

// -----------------------------------------------------------------------------
void runTest(const std::string& equation, const DataPath& targetArrayPath, int32 expectedErrorCondition, CalculatorItem::WarningCode expectedWarningCondition,
             const int* expectedNumberOfTuples = nullptr, const double* expectedValue = nullptr, CalculatorParameter::AngleUnits units = CalculatorParameter::AngleUnits::Radians)
{
  std::cout << "  Testing equation: ==>" << equation << "<==" << std::endl;

  ArrayCalculatorFilter filter;
  DataStructure dataStructure = ::createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath({k_AttributeMatrix}), equation, units}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(targetArrayPath));

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  if(expectedErrorCondition == static_cast<int32>(CalculatorItem::ErrorCode::Success))
  {
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  else
  {
    COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == expectedErrorCondition);
  }

  if(expectedWarningCondition != CalculatorItem::WarningCode::None)
  {
    REQUIRE(executeResult.result.warnings()[0].code == static_cast<int32>(expectedWarningCondition));
  }

  Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(targetArrayPath);

  if(nullptr != expectedNumberOfTuples)
  {
    double expectedNumTuples = *expectedNumberOfTuples;
    REQUIRE(arrayPtr->getNumberOfTuples() == expectedNumTuples);
  }

  if(nullptr != expectedValue && nullptr != expectedNumberOfTuples)
  {
    double value = *expectedValue;
    for(size_t i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(arrayPtr->at(i), value, 0.01));
    }
  }
}

// -----------------------------------------------------------------------------
void MultiComponentArrayCalculatorTest()
{
  ArrayCalculatorFilter filter;
  DataStructure dataStructure = ::createDataStructure();

  SECTION("Multi-Component All Components")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("MultiComponent Array1 + MultiComponent Array2", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* mcArray1 = dataStructure.getDataAs<UInt32Array>(k_MultiComponentArray1Path);
    UInt32Array* mcArray2 = dataStructure.getDataAs<UInt32Array>(k_MultiComponentArray2Path);
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == mcArray1->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == mcArray1->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = arrayPtr->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == mcArray1->at(index) + mcArray2->at(index));
      }
    }
  }

  SECTION("Multi-Component Single Component")
  {
    IFilter::ExecuteResult results =
        createAndExecuteArrayCalculatorFilter("MultiComponent Array1[1] + MultiComponent Array2[0]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* mcArray1 = dataStructure.getDataAs<UInt32Array>(k_MultiComponentArray1Path);
    UInt32Array* mcArray2 = dataStructure.getDataAs<UInt32Array>(k_MultiComponentArray2Path);
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == mcArray1->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == 1);
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      int index1 = mcArray1->getNumberOfComponents() * t + 1;
      int index2 = mcArray2->getNumberOfComponents() * t + 0;
      int arrayIndex = arrayPtr->getNumberOfComponents() * t + 0;
      REQUIRE(arrayPtr->at(arrayIndex) == mcArray1->at(index1) + mcArray2->at(index2));
    }
  }

  SECTION("Multi-Component With Scalar")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\" + 2", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArray->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == nArray->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = nArray->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == nArray->at(index) + 2);
      }
    }
  }

  SECTION("Multi-Component All Components Number/Sign Array Names")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\" + \"*\"", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArray->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == nArray->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = nArray->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == nArray->at(index) + sArray->at(index));
      }
    }
  }

  SECTION("Multi-Component Single Components Number/Sign Array Names")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\"[0] + \"*\"[1]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArray->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == 1);
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      int nIndex = nArray->getNumberOfComponents() * t + 0;
      int sIndex = sArray->getNumberOfComponents() * t + 1;
      REQUIRE(arrayPtr->at(t) == nArray->at(nIndex) + sArray->at(sIndex));
    }
  }

  SECTION("Multi-Component Inconsistent indexing error")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\" + \"*\"[1]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);

    COMPLEX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorItem::ErrorCode::InconsistentCompDims));
  }

  SECTION("Multi-Component Out of bounds error")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\"[0] + \"*\"[3]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);

    COMPLEX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorItem::ErrorCode::ComponentOutOfRange));
  }
}

// -----------------------------------------------------------------------------
void SingleComponentArrayCalculatorTest1()
{
  SECTION("Empty Tests")
  {
    runTest("", k_NumericArrayPath, FilterParameter::Constants::k_Validate_Empty_Value, CalculatorItem::WarningCode::None);
    runTest("          ", k_NumericArrayPath, FilterParameter::Constants::k_Validate_Empty_Value, CalculatorItem::WarningCode::None);
  }

  SECTION("Single Value Tests")
  {
    int numTuple = 1;
    double value = -3;
    runTest("-3", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    numTuple = 1;
    value = 14;
    runTest("14", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    numTuple = 1;
    value = 0.345;
    runTest(".345", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Mismatched Parentheses Tests")
  {
    int numTuple = 1;
    double value = 12;
    runTest("(3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
    runTest("(3*4", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::MismatchedParentheses), CalculatorItem::WarningCode::None);
    runTest("3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::MismatchedParentheses), CalculatorItem::WarningCode::None);
  }

  SECTION("Nested Unary Operator Test")
  {
    int numTuple = 1;
    double value = sin(pow(fabs(cos(fabs(static_cast<double>(3)) / 4) + 7), 2));
    runTest("sin( abs( cos( abs(3)/4) + 7)^2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Single Array Tests (Force Incorrect Tuple Counts)")
  {
    runTest("-InputArray1", k_NumericArrayPath, -265, CalculatorItem::WarningCode::None);
    runTest(k_InputArray2, k_NumericArrayPath, -265, CalculatorItem::WarningCode::None);

    int numTuple = 10;
    double value = 18;
    runTest("12 + 6", k_AttributeArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Unrecognized Item Tests")
  {
    runTest("-foo", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::UnrecognizedItem), CalculatorItem::WarningCode::None);
    runTest("InputArray3", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::UnrecognizedItem), CalculatorItem::WarningCode::None);
    runTest("sin(InputArray 2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::UnrecognizedItem), CalculatorItem::WarningCode::None);
  }

  // Operator Tests

  SECTION("Addition Operator")
  {
    runTest("+", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);
    runTest("3 +", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);
    runTest("+ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 18;
    runTest("12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -6;
    runTest("-12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
    runTest("6 + -12", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Subtraction Operator")
  {
    runTest("-89.2 -", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 43;
    runTest("97 - 54", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -34;
    runTest("-32 - 2", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 19;
    runTest("7 - -12", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Multiplication Operator")
  {
    runTest("*", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);
    runTest("3 *", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);
    runTest("* 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 72;
    runTest("12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = -72;
    runTest("-12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::AmbiguousNameWarning, &numTuple, &value);
    runTest("6 * -12", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Division Operator")
  {
    runTest("/", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);
    runTest("3 /", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);
    runTest("/ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 2;
    runTest("12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -2;
    runTest("-12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -0.5;
    runTest("6 / -12", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Pow Operator")
  {
    runTest("^", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);
    runTest("3 ^", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);
    runTest("^ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoLeftValue), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 125;
    runTest("5 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -8;
    runTest("-2 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 0.25;
    runTest("2 ^ -2", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Abs Operator")
  {
    runTest("abs", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("abs(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("abs)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("abs()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 2;
    runTest("abs(2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 4.3;
    runTest("abs(-4.3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 6.7;
    runTest("abs(abs(6.7))", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Sin Operator")
  {
    runTest("sin", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("sin(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("sin)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("sin()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("sin(90)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0;
    runTest("sin(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    runTest("sin(" + k_Pi_Str + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = 1;
    runTest("sin(" + k_Pi_Str + "/2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("Cos Operator")
  {
    runTest("cos", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("cos(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("cos)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("cos()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 0;
    runTest("cos(90)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = -1;
    runTest("cos(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    runTest("cos(" + k_Pi_Str + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = -0.5;
    runTest("cos(2*" + k_Pi_Str + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("Tan Operator")
  {
    runTest("tan", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("tan(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("tan)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("tan()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("tan(45)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = sqrt(3);
    runTest("tan(60)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 1;
    runTest("tan(" + k_Pi_Str + "/4)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = -sqrt(static_cast<double>(1) / static_cast<double>(3));
    runTest("tan(5*" + k_Pi_Str + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("ASin Operator")
  {
    runTest("asin", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("asin(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("asin)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("asin()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 30;
    runTest("asin(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    runTest("asin(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Degrees);

    value = numbers::pi / 3;
    runTest("asin(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = numbers::pi / 2;
    runTest("asin(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ACos Operator")
  {
    runTest("acos", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("acos(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("acos)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("acos()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 60;
    runTest("acos(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    runTest("acos(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Degrees);

    value = numbers::pi / 6;
    runTest("acos(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = 0;
    runTest("acos(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ATan Operator")
  {
    runTest("atan", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("atan(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("atan)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("atan()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = -45;
    runTest("atan(-1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value, CalculatorParameter::Degrees);

    value = -60;
    runTest("atan(-sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Degrees);

    value = numbers::pi / 6;
    runTest("atan(1/sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = numbers::pi / 3;
    runTest("atan(sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("Sqrt Operator")
  {
    runTest("sqrt", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("sqrt(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("sqrt)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("sqrt()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("sqrt(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 3;
    runTest("sqrt(9)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 4;
    runTest("sqrt(4*4)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 3;
    runTest("sqrt(3^2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Root Operator")
  {
    runTest("root", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("root(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("root)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("root()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NotEnoughArguments), CalculatorItem::WarningCode::None);
    runTest("root(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NotEnoughArguments), CalculatorItem::WarningCode::None);
    runTest("root(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 3;
    runTest("root(9, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 4;
    runTest("root(4*4, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 4;
    runTest("root(4*4+0, 1*2+0)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 4;
    runTest("root(64, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Log10 Operator")
  {
    runTest("log10", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("log10(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("log10)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("log10()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("log10(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);
    runTest("log10(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = log10(10);
    runTest("log10(10)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = log10(40);
    runTest("log10(40)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Log Operator")
  {
    runTest("log", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("log(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("log)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("log()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NotEnoughArguments), CalculatorItem::WarningCode::None);
    runTest("log(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NotEnoughArguments), CalculatorItem::WarningCode::None);
    runTest("log(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = log(5) / log(2);
    runTest("log(2, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 2;
    runTest("log(10, 100)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Exp Operator")
  {
    runTest("exp", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("exp(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("exp)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("exp()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("exp(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);
    runTest("exp(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 2.7182818284590452354; // M_E
    runTest("exp(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 1;
    runTest("exp(0)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Ln Operator")
  {
    runTest("ln", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("ln(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("ln)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("ln()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("ln(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);
    runTest("ln(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = log(1);
    runTest("ln(1)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = log(7);
    runTest("ln(7)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Floor Operator")
  {
    runTest("floor", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("floor(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("floor)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("floor()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("floor(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);
    runTest("floor(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 12;
    runTest("floor(12.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -83;
    runTest("floor(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Ceil Operator")
  {
    runTest("ceil", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("ceil(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoClosingParen), CalculatorItem::WarningCode::None);
    runTest("ceil)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoOpeningParen), CalculatorItem::WarningCode::None);
    runTest("ceil()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("ceil(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);
    runTest("ceil(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::TooManyArguments), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("ceil(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -82;
    runTest("ceil(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }

  SECTION("Negative Operator")
  {
    runTest("-", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);

    runTest("-(", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::MismatchedParentheses), CalculatorItem::WarningCode::None);
    runTest("-)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::OperatorNoRightValue), CalculatorItem::WarningCode::None);
    runTest("-()", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoNumericArguments), CalculatorItem::WarningCode::None);
    runTest("-(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoPrecedingUnaryOperator), CalculatorItem::WarningCode::None);
    runTest("-(,)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::NoPrecedingUnaryOperator), CalculatorItem::WarningCode::None);

    int numTuple = 1;
    double value = -9;
    runTest("- 9", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = -0.4564;
    runTest("-(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);

    value = 1;
    runTest("-(3-4)", k_NumericArrayPath, static_cast<int32>(CalculatorItem::ErrorCode::Success), CalculatorItem::WarningCode::NumericValueWarning, &numTuple, &value);
  }
}

// -----------------------------------------------------------------------------
void SingleComponentArrayCalculatorTest2()
{
  ArrayCalculatorFilter filter;
  DataStructure dataStructure = createDataStructure();

  // Single Array Tests
  SECTION("Single Array Negative Operator")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("-InputArray1", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32Array* inputArray1 = dataStructure.getDataAs<Float32Array>(DataPath({k_AttributeMatrix, k_InputArray1}));
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray1->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray1->at(i) * -1);
    }
  }

  SECTION("Single Array No Operator")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter(k_InputArray2, k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* inputArray2 = dataStructure.getDataAs<UInt32Array>(DataPath({k_AttributeMatrix, k_InputArray2}));
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray2->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray2->at(i));
    }
  }

  // Multiple Array Tests
  SECTION("Multiple Array Addition")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("Spaced Array + InputArray1", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32Array* inputArray1 = dataStructure.getDataAs<Float32Array>(DataPath({k_AttributeMatrix, k_InputArray1}));
    UInt32Array* spacedArray = dataStructure.getDataAs<UInt32Array>(DataPath({k_AttributeMatrix, k_SpacedArray}));
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == spacedArray->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray1->at(i) + spacedArray->at(i));
    }
  }
  SECTION("Multiple Array Multiple Operators")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("sqrt((InputArray1^2)+(InputArray2^2))", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32Array* inputArray1 = dataStructure.getDataAs<Float32Array>(DataPath({k_AttributeMatrix, k_InputArray1}));
    UInt32Array* inputArray2 = dataStructure.getDataAs<UInt32Array>(DataPath({k_AttributeMatrix, k_InputArray2}));
    Float64Array* arrayPtr = dataStructure.getDataAs<Float64Array>(k_AttributeArrayPath);

    COMPLEX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray2->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      double value = pow(inputArray1->at(i), 2) + pow(inputArray2->at(i), 2);
      value = sqrt(value);
      REQUIRE(arrayPtr->at(i) == value);
    }
  }
}

TEST_CASE("ComplexCore::ArrayCalculatorFilter: Filter Execution")
{
  std::cout << "#### ArrayCalculatorTest Starting ####" << std::endl;

  SingleComponentArrayCalculatorTest1();
  SingleComponentArrayCalculatorTest2();
  MultiComponentArrayCalculatorTest();
}
