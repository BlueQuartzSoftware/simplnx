#include "SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp"
#include "SimplnxCore/Filters/ArrayCalculatorFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>
#include <numbers>

using namespace nx::core;

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
void runTest(const std::string& equation, const DataPath& targetArrayPath, int32 expectedErrorCondition, CalculatorWarningCode expectedWarningCondition, const int* expectedNumberOfTuples = nullptr,
             const double* expectedValue = nullptr, CalculatorParameter::AngleUnits units = CalculatorParameter::AngleUnits::Radians)
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
  if(expectedErrorCondition == static_cast<int32>(CalculatorErrorCode::Success))
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == expectedErrorCondition);
  }

  if(executeResult.result.warnings().size() != 0)
  {
    REQUIRE(expectedWarningCondition != CalculatorWarningCode::None);
    REQUIRE(executeResult.result.warnings()[0].code == static_cast<int32>(expectedWarningCondition));
  }
  else
  {
    REQUIRE(expectedWarningCondition == CalculatorWarningCode::None);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::InconsistentCompDims));
  }

  SECTION("Multi-Component Out of bounds error")
  {
    IFilter::ExecuteResult results = createAndExecuteArrayCalculatorFilter("\"4\"[0] + \"*\"[3]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32Array* nArray = dataStructure.getDataAs<UInt32Array>(k_NumberArrayPath);
    UInt32Array* sArray = dataStructure.getDataAs<UInt32Array>(k_SignArrayPath);

    SIMPLNX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::ComponentOutOfRange));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
void SingleComponentArrayCalculatorTest1()
{
  SECTION("Empty Tests")
  {
    runTest("", k_NumericArrayPath, FilterParameter::Constants::k_Validate_Empty_Value, CalculatorWarningCode::None);
    runTest("          ", k_NumericArrayPath, FilterParameter::Constants::k_Validate_Empty_Value, CalculatorWarningCode::None);
  }

  SECTION("Single Value Tests")
  {
    int numTuple = 1;
    double value = -3;
    runTest("-3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    numTuple = 1;
    value = 14;
    runTest("14", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    numTuple = 1;
    value = 0.345;
    runTest(".345", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Mismatched Parentheses Tests")
  {
    int numTuple = 1;
    double value = 12;
    runTest("(3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
    runTest("(3*4", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
    runTest("3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
  }

  SECTION("Nested Unary Operator Test")
  {
    int numTuple = 1;
    float64 value = sin(pow(fabs(cos(fabs(static_cast<float64>(3)) / 4) + 7), 2));
    runTest("sin( abs( cos( abs(3)/4) + 7)^2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    // term1 = (12.5 * (3.14 + 2.718)) / (7 - (8 * (9 + 4)))
    float64 term1 = (12.5 * (3.14 + 2.718)) / (7.0 - (8.0 * (9.0 + 4.0)));

    // term2_1 = sin(pi / (4 * (2 + 6)))
    float64 term2_1 = std::sin(3.141592653589793 / (4.0 * (2.0 + 6.0)));

    // term2_2 = pow(base, 1/expArg) where
    //    base   = 5^(1+2)
    //    expArg = 10/((3^2)+1)
    float64 base = std::pow(5.0, 1.0 + 2.0);
    float64 expArg = 10.0 / ((std::pow(3.0, 2.0) + 1.0));
    float64 term2_2 = std::pow(base, 1.0 / expArg);

    float64 term2 = std::max(term2_1, term2_2);

    // term3 = abs(-((15+3) - (20/(5+5))))
    float64 term3 = std::abs(-((15.0 + 3.0) - (20.0 / (5.0 + 5.0))));

    value = std::min(term1, term2) + term3;

    runTest("min(((12.5*(3.14+2.718))/(7-(8*(9+4)))),max(sin(3.141592653589793/(4*(2+6))),root((5^(1+2)),(10/((3^2)+1)))))+abs(-((15+3)-(20/(5+5))))", k_NumericArrayPath,
            static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Single Array Tests (Force Incorrect Tuple Counts)")
  {
    runTest("-InputArray1", k_NumericArrayPath, -268, CalculatorWarningCode::None);
    runTest(k_InputArray2, k_NumericArrayPath, -268, CalculatorWarningCode::None);

    int numTuple = 10;
    double value = 18;
    runTest("12 + 6", k_AttributeArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Unrecognized Item Tests")
  {
    runTest("-foo", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
    runTest("InputArray3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
    runTest("sin(InputArray 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
  }

  // Operator Tests

  SECTION("Addition Operator")
  {
    runTest("+", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    runTest("3 +", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    runTest("+ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 18;
    runTest("12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -6;
    runTest("-12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
    runTest("6 + -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Subtraction Operator")
  {
    runTest("-89.2 -", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 43;
    runTest("97 - 54", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -34;
    runTest("-32 - 2", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 19;
    runTest("7 - -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Multiplication Operator")
  {
    runTest("*", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    runTest("3 *", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    runTest("* 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 72;
    runTest("12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = -72;
    runTest("-12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
    runTest("6 * -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Division Operator")
  {
    runTest("/", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    runTest("3 /", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    runTest("/ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    runTest("12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -2;
    runTest("-12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -0.5;
    runTest("6 / -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Pow Operator")
  {
    runTest("^", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    runTest("3 ^", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    runTest("^ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 125;
    runTest("5 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -8;
    runTest("-2 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 0.25;
    runTest("2 ^ -2", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Abs Operator")
  {
    runTest("abs", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("abs(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("abs)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("abs()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    runTest("abs(2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4.3;
    runTest("abs(-4.3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 6.7;
    runTest("abs(abs(6.7))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Sin Operator")
  {
    runTest("sin", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("sin(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("sin)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("sin()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("sin(90)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0;
    runTest("sin(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    runTest("sin(" + k_Pi_Str + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = 1;
    runTest("sin(" + k_Pi_Str + "/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("Cos Operator")
  {
    runTest("cos", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("cos(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("cos)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("cos()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 0;
    runTest("cos(90)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = -1;
    runTest("cos(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    runTest("cos(" + k_Pi_Str + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = -0.5;
    runTest("cos(2*" + k_Pi_Str + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("Tan Operator")
  {
    runTest("tan", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("tan(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("tan)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("tan()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("tan(45)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = sqrt(3);
    runTest("tan(60)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 1;
    runTest("tan(" + k_Pi_Str + "/4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = -sqrt(static_cast<double>(1) / static_cast<double>(3));
    runTest("tan(5*" + k_Pi_Str + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("ASin Operator")
  {
    runTest("asin", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("asin(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("asin)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("asin()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 30;
    runTest("asin(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    runTest("asin(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::pi / 3;
    runTest("asin(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = numbers::pi / 2;
    runTest("asin(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ACos Operator")
  {
    runTest("acos", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("acos(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("acos)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("acos()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 60;
    runTest("acos(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    runTest("acos(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::pi / 6;
    runTest("acos(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = 0;
    runTest("acos(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ATan Operator")
  {
    runTest("atan", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("atan(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("atan)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("atan()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = -45;
    runTest("atan(-1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = -60;
    runTest("atan(-sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::pi / 6;
    runTest("atan(1/sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = numbers::pi / 3;
    runTest("atan(sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("Sqrt Operator")
  {
    runTest("sqrt", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("sqrt(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("sqrt)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("sqrt()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("sqrt(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 3;
    runTest("sqrt(9)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4;
    runTest("sqrt(4*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 3;
    runTest("sqrt(3^2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Root Operator")
  {
    runTest("root", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("root(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("root)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("root()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("root(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("root(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 3;
    runTest("root(9, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4;
    runTest("root(4*4, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 4;
    runTest("root(4*4+0, 1*2+0)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 4;
    runTest("root(64, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Log10 Operator")
  {
    runTest("log10", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("log10(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("log10)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("log10()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("log10(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    runTest("log10(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log10(10);
    runTest("log10(10)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = log10(40);
    runTest("log10(40)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Log Operator")
  {
    runTest("log", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("log(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("log)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("log()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("log(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("log(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log(5) / log(2);
    runTest("log(2, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 2;
    runTest("log(10, 100)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Exp Operator")
  {
    runTest("exp", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("exp(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("exp)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("exp()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("exp(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    runTest("exp(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2.7182818284590452354; // M_E
    runTest("exp(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 1;
    runTest("exp(0)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Ln Operator")
  {
    runTest("ln", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("ln(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("ln)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("ln()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("ln(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    runTest("ln(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log(1);
    runTest("ln(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = log(7);
    runTest("ln(7)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Floor Operator")
  {
    runTest("floor", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("floor(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("floor)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("floor()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("floor(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    runTest("floor(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 12;
    runTest("floor(12.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -83;
    runTest("floor(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Ceil Operator")
  {
    runTest("ceil", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("ceil(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("ceil)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("ceil()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("ceil(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    runTest("ceil(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    runTest("ceil(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -82;
    runTest("ceil(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Negative Operator")
  {
    runTest("-", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);

    runTest("-(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
    runTest("-)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    runTest("-()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    runTest("-(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoPrecedingUnaryOperator), CalculatorWarningCode::None);
    runTest("-(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoPrecedingUnaryOperator), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = -9;
    runTest("- 9", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -0.4564;
    runTest("-(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 1;
    runTest("-(3-4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Min Operator")
  {
    runTest("min", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("min(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("min)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("min()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("min(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("min(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    runTest("min(2,6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -93;
    runTest("min(-82,-93)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Max Operator")
  {
    runTest("max", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("max(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    runTest("max)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    runTest("max()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("max(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    runTest("max(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 6;
    runTest("max(2,6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -82;
    runTest("max(-82,-93)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
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

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray2->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      double value = pow(inputArray1->at(i), 2) + pow(inputArray2->at(i), 2);
      value = sqrt(value);
      REQUIRE(arrayPtr->at(i) == value);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Filter Execution")
{
  UnitTest::LoadPlugins();

  std::cout << "#### ArrayCalculatorTest Starting ####" << std::endl;

  SingleComponentArrayCalculatorTest1();
  SingleComponentArrayCalculatorTest2();
  MultiComponentArrayCalculatorTest();
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Tokenizer")
{
  using TT = nx::core::TokenType;

  SECTION("Simple arithmetic")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("3 + 4.5");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == TT::Number);
    REQUIRE(tokens[0].text == "3");
    REQUIRE(tokens[0].position == 0);
    REQUIRE(tokens[1].type == TT::Plus);
    REQUIRE(tokens[1].position == 2);
    REQUIRE(tokens[2].type == TT::Number);
    REQUIRE(tokens[2].text == "4.5");
    REQUIRE(tokens[2].position == 4);
  }

  SECTION("Single-word identifiers")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("Spaced Array + 1");
    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[0].type == TT::Identifier);
    REQUIRE(tokens[0].text == "Spaced");
    REQUIRE(tokens[1].type == TT::Identifier);
    REQUIRE(tokens[1].text == "Array");
    REQUIRE(tokens[2].type == TT::Plus);
    REQUIRE(tokens[3].type == TT::Number);
    REQUIRE(tokens[3].text == "1");
  }

  SECTION("Quoted string")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("\"CellData/Confidence Index\" + 1");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == TT::QuotedString);
    REQUIRE(tokens[0].text == "CellData/Confidence Index");
    REQUIRE(tokens[1].type == TT::Plus);
    REQUIRE(tokens[2].type == TT::Number);
  }

  SECTION("Brackets and comma")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("Array[3, 2]");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TT::Identifier);
    REQUIRE(tokens[0].text == "Array");
    REQUIRE(tokens[1].type == TT::LBracket);
    REQUIRE(tokens[2].type == TT::Number);
    REQUIRE(tokens[2].text == "3");
    REQUIRE(tokens[3].type == TT::Comma);
    REQUIRE(tokens[4].type == TT::Number);
    REQUIRE(tokens[4].text == "2");
    REQUIRE(tokens[5].type == TT::RBracket);
  }

  SECTION("All operator tokens")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("+ - * / ^ %");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TT::Plus);
    REQUIRE(tokens[1].type == TT::Minus);
    REQUIRE(tokens[2].type == TT::Star);
    REQUIRE(tokens[3].type == TT::Slash);
    REQUIRE(tokens[4].type == TT::Caret);
    REQUIRE(tokens[5].type == TT::Percent);
  }

  SECTION("Decimal starting with dot")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize(".345");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == TT::Number);
    REQUIRE(tokens[0].text == ".345");
  }

  SECTION("Complex expression")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("sin(pi / 2)");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TT::Identifier);
    REQUIRE(tokens[0].text == "sin");
    REQUIRE(tokens[1].type == TT::LParen);
    REQUIRE(tokens[2].type == TT::Identifier);
    REQUIRE(tokens[2].text == "pi");
    REQUIRE(tokens[3].type == TT::Slash);
    REQUIRE(tokens[4].type == TT::Number);
    REQUIRE(tokens[4].text == "2");
    REQUIRE(tokens[5].type == TT::RParen);
  }

  SECTION("Negative number tokenizes as minus + number")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("-3.14");
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == TT::Minus);
    REQUIRE(tokens[1].type == TT::Number);
    REQUIRE(tokens[1].text == "3.14");
  }

  SECTION("Empty string")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("");
    REQUIRE(tokens.empty());
  }

  SECTION("Parentheses")
  {
    auto tokens = nx::core::ArrayCalculatorParser::tokenize("(3+4)");
    REQUIRE(tokens.size() == 5);
    REQUIRE(tokens[0].type == TT::LParen);
    REQUIRE(tokens[4].type == TT::RParen);
  }
}

// -----------------------------------------------------------------------------
// Test 1: Array Resolution
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Array Resolution")
{
  UnitTest::LoadPlugins();

  // Create a DataStructure with arrays in multiple groups.
  // "Group1/SharedName" (UInt32, 10 tuples, filled with 5)
  // "Group2/SharedName" (UInt32, 10 tuples, filled with 7)
  // "Group1/UniqueName" (UInt32, 10 tuples, filled with 3)
  DataStructure ds;
  AttributeMatrix* group1 = AttributeMatrix::Create(ds, "Group1", {10ULL});
  auto group1Id = group1->getId();
  AttributeMatrix* group2 = AttributeMatrix::Create(ds, "Group2", {10ULL});
  auto group2Id = group2->getId();

  UInt32Array* shared1 = UInt32Array::CreateWithStore<UInt32DataStore>(ds, "SharedName", {10}, {1}, group1Id);
  shared1->fill(5);
  UInt32Array* shared2 = UInt32Array::CreateWithStore<UInt32DataStore>(ds, "SharedName", {10}, {1}, group2Id);
  shared2->fill(7);
  UInt32Array* unique1 = UInt32Array::CreateWithStore<UInt32DataStore>(ds, "UniqueName", {10}, {1}, group1Id);
  unique1->fill(3);

  ArrayCalculatorFilter filter;

  SECTION("Unique bare name resolves without selected group")
  {
    DataPath outputPath({"Group1", "NewArray"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath{}, "UniqueName + 1", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(outputPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 4.0, 0.01));
    }
  }

  SECTION("Ambiguous bare name with no selected group gives error")
  {
    DataPath outputPath({"Group1", "AmbiguousResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath{}, "SharedName + 1", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::AmbiguousArrayName));
  }

  SECTION("Selected group resolves ambiguity")
  {
    DataPath outputPath({"Group1", "ResolvedResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath({"Group1"}), "SharedName + 1", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(outputPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 6.0, 0.01));
    }
  }

  SECTION("Quoted full path resolves directly")
  {
    DataPath outputPath({"Group2", "QuotedResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{DataPath{}, "\"Group2/SharedName\" + 1", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(outputPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 8.0, 0.01));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// Test 2: Built-in Constants
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Built-in Constants")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("pi resolves to std::numbers::pi")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "pi", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), std::numbers::pi, 0.0001));
    }
  }

  SECTION("e resolves to std::numbers::e")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "e", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), std::numbers::e, 0.0001));
    }
  }

  SECTION("2 * pi expression works")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "2 * pi", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 2.0 * std::numbers::pi, 0.0001));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// Test 3: Modulo Operator
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Modulo Operator")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("Scalar modulo: 10 % 3 = 1")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "10 % 3", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 1.0, 0.01));
    }
  }

  SECTION("Array modulo: InputArray2 % 3 = 1 element-wise")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "InputArray2 % 3", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 1.0, 0.01));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// Test 4: Tuple+Component Indexing
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Tuple Component Indexing")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("MultiComponent Array1[2, 1] produces scalar value 7")
  {
    // MultiComponent Array1 has 10 tuples x 3 comps, values 0..29 sequentially.
    // tuple 2, comp 1 = index 2*3 + 1 = 7
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "\"MultiComponent Array1\"[2, 1]", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    // Scalar result broadcast to all tuples in the AttributeMatrix (10 tuples)
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 7.0, 0.01));
    }
  }

  SECTION("MultiComponent Array1[100, 0] out of bounds gives TupleOutOfRange error")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "\"MultiComponent Array1\"[100, 0]", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::TupleOutOfRange));
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// Test 5: Sub-expression Component Access
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Sub-expression Component Access")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("(MultiComponent Array1 + MultiComponent Array2)[0] extracts component 0 of the sum")
  {
    // MultiComponent Array1 and Array2 both have 10 tuples x 3 components, values 0..29.
    // Sum = 2*values = [0,2,4,6,8,10,...,58]
    // Component 0 extraction: for each tuple t, take element at (t*3 + 0) = 2*(t*3)
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              k_AttributeMatrixPath, "(\"MultiComponent Array1\" + \"MultiComponent Array2\")[0]", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    REQUIRE(outputArray.getNumberOfComponents() == 1);
    for(usize t = 0; t < 10; t++)
    {
      // Component 0 of sum: (t*3 + 0) + (t*3 + 0) = 2 * (t * 3)
      double expected = 2.0 * static_cast<double>(t * 3);
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(t), expected, 0.01));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// Test 6: Multi-word Array Names
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Multi-word Array Names")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("Spaced Array + 1 = 3")
  {
    // Spaced Array is filled with 2, so Spaced Array + 1 = 3
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{k_AttributeMatrixPath, "Spaced Array + 1", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), 3.0, 0.01));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Sub-expression Tuple Component Extraction")
{
  UnitTest::LoadPlugins();
  DataStructure ds = ::createDataStructure();
  ArrayCalculatorFilter filter;

  // MultiComponent Array1 has 10 tuples, 3 components, values 0,1,2,3,...,29
  // (ArrayA + ArrayB) at tuple 2, component 1 = 2*(2*3+1) = 14

  SECTION("(expr)[T, C] produces scalar")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              k_AttributeMatrixPath, "(\"MultiComponent Array1\" + \"MultiComponent Array2\")[2, 1]", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(k_AttributeArrayPath));
    const auto& outputArray = ds.getDataRefAs<Float64Array>(k_AttributeArrayPath);
    // Scalar result broadcast to AM shape (10 tuples)
    double expected = 2.0 * (2 * 3 + 1); // tuple 2, comp 1, doubled = 14
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(UnitTest::CloseEnough<double>(outputArray.at(i), expected, 0.01));
    }
  }

  SECTION("(expr)[T, C] out of bounds tuple")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              k_AttributeMatrixPath, "(\"MultiComponent Array1\" + \"MultiComponent Array2\")[100, 0]", CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::TupleOutOfRange));
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}
