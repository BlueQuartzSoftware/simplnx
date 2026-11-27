#include "SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp"
#include "SimplnxCore/Filters/ArrayCalculatorFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <optional>
#include <utility>

using namespace nx::core;
namespace fs = std::filesystem;

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

const std::string k_PiStr = string_utilities::Number(numbers::k_Pi);

template <typename T>
class ArrayCalculatorFailOnLaterReadStore : public DataStore<T>
{
public:
  ArrayCalculatorFailOnLaterReadStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode, usize failingRead)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  , m_FailingRead(failingRead)
  {
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<T> buffer) const override
  {
    if(++m_ReadCount == m_FailingRead)
    {
      return MakeErrorResult(m_ErrorCode, "Injected ArrayCalculator later-page read failure");
    }
    return DataStore<T>::copyIntoBuffer(offset, buffer);
  }

private:
  int32 m_ErrorCode;
  usize m_FailingRead;
  mutable usize m_ReadCount = 0;
};

// -----------------------------------------------------------------------------
DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  AttributeMatrix* am1Ptr = AttributeMatrix::Create(dataStructure, k_AttributeMatrix, {10ULL});
  const auto am1Id = am1Ptr->getId();
  AttributeMatrix* am2Ptr = AttributeMatrix::Create(dataStructure, k_NumericMatrix, {1ULL});
  Float32ArrayType* array1Ptr = Float32ArrayType::CreateWithStore<Float32DataStoreType>(dataStructure, k_InputArray1, {10}, {1}, am1Id);
  array1Ptr->fill(-12);
  UInt32ArrayType* array2Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, k_InputArray2, {10}, {1}, am1Id);
  array2Ptr->fill(10);
  UInt32ArrayType* sArrayPtr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, k_SpacedArray, {10}, {1}, am1Id);
  sArrayPtr->fill(2);

  UInt32ArrayType* mcArray1Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, k_MultiComponentArray1, std::vector<size_t>(1, 10), std::vector<size_t>(1, 3), am1Id);
  int num = 0;
  for(int i = 0; i < mcArray1Ptr->getNumberOfTuples() * mcArray1Ptr->getNumberOfComponents(); i++)
  {
    (*mcArray1Ptr)[i] = num;
    num++;
  }

  UInt32ArrayType* mcArray2Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, k_MultiComponentArray2, std::vector<size_t>(1, 10), std::vector<size_t>(1, 3), am1Id);
  num = 0;
  for(int i = 0; i < mcArray2Ptr->getNumberOfTuples() * mcArray2Ptr->getNumberOfComponents(); i++)
  {
    (*mcArray2Ptr)[i] = num;
    num++;
  }

  auto numberArray = mcArray2Ptr->deepCopy(k_NumberArrayPath);
  auto signArray = mcArray1Ptr->deepCopy(k_SignArrayPath);

  return dataStructure;
}

DataStructure CreateComponentShapeDataStructure()
{
  DataStructure dataStructure;
  auto* amPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrix, {1});
  REQUIRE(amPtr != nullptr);
  auto* arrayAPtr = Float64ArrayType::CreateWithStore<Float64DataStoreType>(dataStructure, "A", {1}, {3}, amPtr->getId());
  auto* arrayBPtr = Float64ArrayType::CreateWithStore<Float64DataStoreType>(dataStructure, "B", {1}, {3}, amPtr->getId());
  REQUIRE(arrayAPtr != nullptr);
  REQUIRE(arrayBPtr != nullptr);
  (*arrayAPtr)[0] = 1;
  (*arrayAPtr)[1] = 2;
  (*arrayAPtr)[2] = 4;
  (*arrayBPtr)[0] = 10;
  (*arrayBPtr)[1] = 20;
  (*arrayBPtr)[2] = 40;
  return dataStructure;
}
} // namespace

// -----------------------------------------------------------------------------
static IFilter::ExecuteResult CreateAndExecuteArrayCalculatorFilter(const std::string& equation, const DataPath& calculatedPath, const CalculatorParameter::AngleUnits& units,
                                                                    DataStructure& dataStructure, ArrayCalculatorFilter& filter)
{
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                      std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{.m_selected_group = DataPath({k_AttributeMatrix}), .equation = equation, .m_units = units}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(calculatedPath));

  return filter.execute(dataStructure, args);
}

// -----------------------------------------------------------------------------
static void RunTest(const std::string& equation, const DataPath& targetArrayPath, int32 expectedErrorCondition, CalculatorWarningCode expectedWarningCondition,
                    const int* expectedNumberOfTuples = nullptr, const double* expectedValue = nullptr, CalculatorParameter::AngleUnits units = CalculatorParameter::AngleUnits::Radians)
{
  std::cout << "  Testing equation: ==>" << equation << "<==" << '\n';

  ArrayCalculatorFilter filter;
  DataStructure dataStructure = ::CreateDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                      std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{.m_selected_group = DataPath({k_AttributeMatrix}), .equation = equation, .m_units = units}));
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

  if(!executeResult.result.warnings().empty())
  {
    REQUIRE(expectedWarningCondition != CalculatorWarningCode::None);
    REQUIRE(executeResult.result.warnings()[0].code == static_cast<int32>(expectedWarningCondition));
  }
  else
  {
    REQUIRE(expectedWarningCondition == CalculatorWarningCode::None);
  }

  Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(targetArrayPath);

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
      REQUIRE(unit_test::CloseEnough<double>(arrayPtr->at(i), value, 0.01));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
static void MultiComponentArrayCalculatorTest()
{
  ArrayCalculatorFilter filter;
  DataStructure dataStructure = ::CreateDataStructure();

  SECTION("Multi-Component All Components")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter("MultiComponent Array1 + MultiComponent Array2", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* mcArray1Ptr = dataStructure.getDataAs<UInt32ArrayType>(k_MultiComponentArray1Path);
    UInt32ArrayType* mcArray2Ptr = dataStructure.getDataAs<UInt32ArrayType>(k_MultiComponentArray2Path);
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == mcArray1Ptr->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == mcArray1Ptr->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = arrayPtr->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == mcArray1Ptr->at(index) + mcArray2Ptr->at(index));
      }
    }
  }

  SECTION("Multi-Component Single Component")
  {
    IFilter::ExecuteResult results =
        CreateAndExecuteArrayCalculatorFilter("MultiComponent Array1[1] + MultiComponent Array2[0]", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* mcArray1Ptr = dataStructure.getDataAs<UInt32ArrayType>(k_MultiComponentArray1Path);
    UInt32ArrayType* mcArray2Ptr = dataStructure.getDataAs<UInt32ArrayType>(k_MultiComponentArray2Path);
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == mcArray1Ptr->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == 1);
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      int index1 = mcArray1Ptr->getNumberOfComponents() * t + 1;
      int index2 = mcArray2Ptr->getNumberOfComponents() * t + 0;
      int arrayIndex = arrayPtr->getNumberOfComponents() * t + 0;
      REQUIRE(arrayPtr->at(arrayIndex) == mcArray1Ptr->at(index1) + mcArray2Ptr->at(index2));
    }
  }

  SECTION("Multi-Component With Scalar")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter("\"4\" + 2", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* nArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_NumberArrayPath);
    UInt32ArrayType* sArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_SignArrayPath);
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArrayPtr->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == nArrayPtr->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = nArrayPtr->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == nArrayPtr->at(index) + 2);
      }
    }
  }

  SECTION("Multi-Component All Components Number/Sign Array Names")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter(R"("4" + "*")", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* nArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_NumberArrayPath);
    UInt32ArrayType* sArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_SignArrayPath);
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArrayPtr->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == nArrayPtr->getNumberOfComponents());
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      for(int c = 0; c < arrayPtr->getNumberOfComponents(); c++)
      {
        int index = nArrayPtr->getNumberOfComponents() * t + c;
        REQUIRE(arrayPtr->at(index) == nArrayPtr->at(index) + sArrayPtr->at(index));
      }
    }
  }

  SECTION("Multi-Component Single Components Number/Sign Array Names")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter(R"("4"[0] + "*"[1])", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* nArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_NumberArrayPath);
    UInt32ArrayType* sArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_SignArrayPath);
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == nArrayPtr->getNumberOfTuples());
    REQUIRE(arrayPtr->getNumberOfComponents() == 1);
    for(int t = 0; t < arrayPtr->getNumberOfTuples(); t++)
    {
      int nIndex = nArrayPtr->getNumberOfComponents() * t + 0;
      int sIndex = sArrayPtr->getNumberOfComponents() * t + 1;
      REQUIRE(arrayPtr->at(t) == nArrayPtr->at(nIndex) + sArrayPtr->at(sIndex));
    }
  }

  SECTION("Multi-Component Inconsistent indexing error")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter(R"("4" + "*"[1])", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* nArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_NumberArrayPath);
    UInt32ArrayType* sArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_SignArrayPath);

    SIMPLNX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::InconsistentCompDims));
  }

  SECTION("Multi-Component Out of bounds error")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter(R"("4"[0] + "*"[3])", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* nArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_NumberArrayPath);
    UInt32ArrayType* sArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(k_SignArrayPath);

    SIMPLNX_RESULT_REQUIRE_INVALID(results.result);
    REQUIRE(results.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::ComponentOutOfRange));
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
static void SingleComponentArrayCalculatorTest1()
{
  SECTION("Empty Tests")
  {
    RunTest("", k_NumericArrayPath, filter_parameter::constants::k_ValidateEmptyValue, CalculatorWarningCode::None);
    RunTest("          ", k_NumericArrayPath, filter_parameter::constants::k_ValidateEmptyValue, CalculatorWarningCode::None);
  }

  SECTION("Single Value Tests")
  {
    int numTuple = 1;
    double value = -3;
    RunTest("-3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    numTuple = 1;
    value = 14;
    RunTest("14", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    numTuple = 1;
    value = 0.345;
    RunTest(".345", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Mismatched Parentheses Tests")
  {
    int numTuple = 1;
    double value = 12;
    RunTest("(3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
    RunTest("(3*4", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
    RunTest("3*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
  }

  SECTION("Nested Unary Operator Test")
  {
    int numTuple = 1;
    float64 value = sin(pow(fabs(cos(fabs(static_cast<float64>(3)) / 4) + 7), 2));
    RunTest("sin( abs( cos( abs(3)/4) + 7)^2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    // term1 = (12.5 * (3.14 + 2.718)) / (7 - (8 * (9 + 4)))
    float64 term1 = (12.5 * (3.14 + 2.718)) / (7.0 - (8.0 * (9.0 + 4.0)));

    // term2_1 = sin(pi / (4 * (2 + 6)))
    float64 term21 = std::sin(3.141592653589793 / (4.0 * (2.0 + 6.0)));

    // term2_2 = pow(base, 1/expArg) where
    //    base   = 5^(1+2)
    //    expArg = 10/((3^2)+1)
    float64 base = std::pow(5.0, 1.0 + 2.0);
    float64 expArg = 10.0 / ((std::pow(3.0, 2.0) + 1.0));
    float64 term22 = std::pow(base, 1.0 / expArg);

    float64 term2 = std::max(term21, term22);

    // term3 = abs(-((15+3) - (20/(5+5))))
    float64 term3 = std::abs(-((15.0 + 3.0) - (20.0 / (5.0 + 5.0))));

    value = std::min(term1, term2) + term3;

    RunTest("min(((12.5*(3.14+2.718))/(7-(8*(9+4)))),max(sin(3.141592653589793/(4*(2+6))),root((5^(1+2)),(10/((3^2)+1)))))+abs(-((15+3)-(20/(5+5))))", k_NumericArrayPath,
            static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Single Array Tests (Force Incorrect Tuple Counts)")
  {
    RunTest("-InputArray1", k_NumericArrayPath, -268, CalculatorWarningCode::None);
    RunTest(k_InputArray2, k_NumericArrayPath, -268, CalculatorWarningCode::None);

    int numTuple = 10;
    double value = 18;
    RunTest("12 + 6", k_AttributeArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Unrecognized Item Tests")
  {
    RunTest("-foo", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
    RunTest("InputArray3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
    RunTest("sin(InputArray 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::UnrecognizedItem), CalculatorWarningCode::None);
  }

  // Operator Tests

  SECTION("Addition Operator")
  {
    RunTest("+", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    RunTest("3 +", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    RunTest("+ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 18;
    RunTest("12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -6;
    RunTest("-12 + 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
    RunTest("6 + -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Subtraction Operator")
  {
    RunTest("-89.2 -", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 43;
    RunTest("97 - 54", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -34;
    RunTest("-32 - 2", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 19;
    RunTest("7 - -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Multiplication Operator")
  {
    RunTest("*", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    RunTest("3 *", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    RunTest("* 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 72;
    RunTest("12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = -72;
    RunTest("-12 * 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
    RunTest("6 * -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Division Operator")
  {
    RunTest("/", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    RunTest("3 /", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    RunTest("/ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    RunTest("12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -2;
    RunTest("-12 / 6", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -0.5;
    RunTest("6 / -12", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Pow Operator")
  {
    RunTest("^", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);
    RunTest("3 ^", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    RunTest("^ 12.5", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoLeftValue), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 125;
    RunTest("5 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -8;
    RunTest("-2 ^ 3", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 0.25;
    RunTest("2 ^ -2", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Abs Operator")
  {
    RunTest("abs", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("abs(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("abs)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("abs()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    RunTest("abs(2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4.3;
    RunTest("abs(-4.3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 6.7;
    RunTest("abs(abs(6.7))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Sin Operator")
  {
    RunTest("sin", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("sin(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("sin)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("sin()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    RunTest("sin(90)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0;
    RunTest("sin(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    RunTest("sin(" + k_PiStr + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = 1;
    RunTest("sin(" + k_PiStr + "/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("Cos Operator")
  {
    RunTest("cos", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("cos(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("cos)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("cos()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 0;
    RunTest("cos(90)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = -1;
    RunTest("cos(-180)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 0.5;
    RunTest("cos(" + k_PiStr + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = -0.5;
    RunTest("cos(2*" + k_PiStr + "/3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("Tan Operator")
  {
    RunTest("tan", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("tan(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("tan)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("tan()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    RunTest("tan(45)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = sqrt(3);
    RunTest("tan(60)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 1;
    RunTest("tan(" + k_PiStr + "/4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);

    value = -sqrt(static_cast<double>(1) / static_cast<double>(3));
    RunTest("tan(5*" + k_PiStr + "/6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value,
            CalculatorParameter::Radians);
  }

  SECTION("ASin Operator")
  {
    RunTest("asin", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("asin(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("asin)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("asin()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 30;
    RunTest("asin(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    RunTest("asin(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::k_Pi / 3;
    RunTest("asin(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = numbers::k_Pi / 2;
    RunTest("asin(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ACos Operator")
  {
    RunTest("acos", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("acos(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("acos)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("acos()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 60;
    RunTest("acos(0.5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = 45;
    RunTest("acos(sqrt(2)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::k_Pi / 6;
    RunTest("acos(sqrt(3)/2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = 0;
    RunTest("acos(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("ATan Operator")
  {
    RunTest("atan", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("atan(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("atan)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("atan()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = -45;
    RunTest("atan(-1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = -60;
    RunTest("atan(-sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Degrees);

    value = numbers::k_Pi / 6;
    RunTest("atan(1/sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);

    value = numbers::k_Pi / 3;
    RunTest("atan(sqrt(3))", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value, CalculatorParameter::Radians);
  }

  SECTION("Sqrt Operator")
  {
    RunTest("sqrt", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("sqrt(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("sqrt)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("sqrt()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("sqrt(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 3;
    RunTest("sqrt(9)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4;
    RunTest("sqrt(4*4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 3;
    RunTest("sqrt(3^2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Root Operator")
  {
    RunTest("root", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("root(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("root)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("root()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("root(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("root(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 3;
    RunTest("root(9, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 4;
    RunTest("root(4*4, 2)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 4;
    RunTest("root(4*4+0, 1*2+0)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);

    value = 4;
    RunTest("root(64, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Log10 Operator")
  {
    RunTest("log10", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("log10(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("log10)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("log10()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("log10(1, 3)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    RunTest("log10(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log10(10);
    RunTest("log10(10)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = log10(40);
    RunTest("log10(40)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Log Operator")
  {
    RunTest("log", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("log(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("log)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("log()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("log(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("log(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log(5) / log(2);
    RunTest("log(2, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 2;
    RunTest("log(10, 100)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Exp Operator")
  {
    RunTest("exp", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("exp(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("exp)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("exp()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("exp(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    RunTest("exp(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2.7182818284590452354; // M_E
    RunTest("exp(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 1;
    RunTest("exp(0)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Ln Operator")
  {
    RunTest("ln", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("ln(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("ln)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("ln()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("ln(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    RunTest("ln(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = log(1);
    RunTest("ln(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = log(7);
    RunTest("ln(7)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Floor Operator")
  {
    RunTest("floor", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("floor(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("floor)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("floor()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("floor(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    RunTest("floor(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 12;
    RunTest("floor(12.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -83;
    RunTest("floor(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Ceil Operator")
  {
    RunTest("ceil", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("ceil(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("ceil)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("ceil()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("ceil(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);
    RunTest("ceil(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::TooManyArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 1;
    RunTest("ceil(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -82;
    RunTest("ceil(-82.789367)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Negative Operator")
  {
    RunTest("-", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);

    RunTest("-(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::MismatchedParentheses), CalculatorWarningCode::None);
    RunTest("-)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoRightValue), CalculatorWarningCode::None);
    RunTest("-()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);
    RunTest("-(1, 5)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoPrecedingUnaryOperator), CalculatorWarningCode::None);
    RunTest("-(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoPrecedingUnaryOperator), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = -9;
    RunTest("- 9", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -0.4564;
    RunTest("-(.4564)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = 1;
    RunTest("-(3-4)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::AmbiguousNameWarning, &numTuple, &value);
  }

  SECTION("Min Operator")
  {
    RunTest("min", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("min(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("min)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("min()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("min(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("min(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 2;
    RunTest("min(2,6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -93;
    RunTest("min(-82,-93)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }

  SECTION("Max Operator")
  {
    RunTest("max", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("max(", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoClosingParen), CalculatorWarningCode::None);
    RunTest("max)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::OperatorNoOpeningParen), CalculatorWarningCode::None);
    RunTest("max()", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("max(1)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NotEnoughArguments), CalculatorWarningCode::None);
    RunTest("max(,)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::NoNumericArguments), CalculatorWarningCode::None);

    int numTuple = 1;
    double value = 6;
    RunTest("max(2,6)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);

    value = -82;
    RunTest("max(-82,-93)", k_NumericArrayPath, static_cast<int32>(CalculatorErrorCode::Success), CalculatorWarningCode::None, &numTuple, &value);
  }
}

// -----------------------------------------------------------------------------
static void SingleComponentArrayCalculatorTest2()
{
  ArrayCalculatorFilter filter;
  DataStructure dataStructure = CreateDataStructure();

  // Single Array Tests
  SECTION("Single Array Negative Operator")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter("-InputArray1", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32ArrayType* inputArray1Ptr = dataStructure.getDataAs<Float32ArrayType>(DataPath({k_AttributeMatrix, k_InputArray1}));
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray1Ptr->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray1Ptr->at(i) * -1);
    }
  }

  SECTION("Single Array No Operator")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter(k_InputArray2, k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    UInt32ArrayType* inputArray2Ptr = dataStructure.getDataAs<UInt32ArrayType>(DataPath({k_AttributeMatrix, k_InputArray2}));
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray2Ptr->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray2Ptr->at(i));
    }
  }

  // Multiple Array Tests
  SECTION("Multiple Array Addition")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter("Spaced Array + InputArray1", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32ArrayType* inputArray1Ptr = dataStructure.getDataAs<Float32ArrayType>(DataPath({k_AttributeMatrix, k_InputArray1}));
    UInt32ArrayType* spacedArrayPtr = dataStructure.getDataAs<UInt32ArrayType>(DataPath({k_AttributeMatrix, k_SpacedArray}));
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == spacedArrayPtr->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      REQUIRE(arrayPtr->at(i) == inputArray1Ptr->at(i) + spacedArrayPtr->at(i));
    }
  }
  SECTION("Multiple Array Multiple Operators")
  {
    IFilter::ExecuteResult results = CreateAndExecuteArrayCalculatorFilter("sqrt((InputArray1^2)+(InputArray2^2))", k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);

    Float32ArrayType* inputArray1Ptr = dataStructure.getDataAs<Float32ArrayType>(DataPath({k_AttributeMatrix, k_InputArray1}));
    UInt32ArrayType* inputArray2Ptr = dataStructure.getDataAs<UInt32ArrayType>(DataPath({k_AttributeMatrix, k_InputArray2}));
    Float64ArrayType* arrayPtr = dataStructure.getDataAs<Float64ArrayType>(k_AttributeArrayPath);

    SIMPLNX_RESULT_REQUIRE_VALID(results.result);
    REQUIRE(arrayPtr->getNumberOfTuples() == inputArray2Ptr->getNumberOfTuples());
    for(int i = 0; i < arrayPtr->getNumberOfTuples(); i++)
    {
      double value = pow(inputArray1Ptr->at(i), 2) + pow(inputArray2Ptr->at(i), 2);
      value = sqrt(value);
      REQUIRE(arrayPtr->at(i) == value);
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Filter Execution")
{
  unit_test::LoadPlugins();

  std::cout << "#### ArrayCalculatorTest Starting ####" << '\n';

  SingleComponentArrayCalculatorTest1();
  SingleComponentArrayCalculatorTest2();
  MultiComponentArrayCalculatorTest();
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Tokenizer")
{
  using TtType = nx::core::TokenType;

  SECTION("Simple arithmetic")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("3 + 4.5");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == TtType::Number);
    REQUIRE(tokens[0].text == "3");
    REQUIRE(tokens[0].position == 0);
    REQUIRE(tokens[1].type == TtType::Plus);
    REQUIRE(tokens[1].position == 2);
    REQUIRE(tokens[2].type == TtType::Number);
    REQUIRE(tokens[2].text == "4.5");
    REQUIRE(tokens[2].position == 4);
  }

  SECTION("Single-word identifiers")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("Spaced Array + 1");
    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[0].type == TtType::Identifier);
    REQUIRE(tokens[0].text == "Spaced");
    REQUIRE(tokens[1].type == TtType::Identifier);
    REQUIRE(tokens[1].text == "Array");
    REQUIRE(tokens[2].type == TtType::Plus);
    REQUIRE(tokens[3].type == TtType::Number);
    REQUIRE(tokens[3].text == "1");
  }

  SECTION("Quoted string")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("\"CellData/Confidence Index\" + 1");
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0].type == TtType::QuotedString);
    REQUIRE(tokens[0].text == "CellData/Confidence Index");
    REQUIRE(tokens[1].type == TtType::Plus);
    REQUIRE(tokens[2].type == TtType::Number);
  }

  SECTION("Brackets and comma")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("Array[3, 2]");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TtType::Identifier);
    REQUIRE(tokens[0].text == "Array");
    REQUIRE(tokens[1].type == TtType::LBracket);
    REQUIRE(tokens[2].type == TtType::Number);
    REQUIRE(tokens[2].text == "3");
    REQUIRE(tokens[3].type == TtType::Comma);
    REQUIRE(tokens[4].type == TtType::Number);
    REQUIRE(tokens[4].text == "2");
    REQUIRE(tokens[5].type == TtType::RBracket);
  }

  SECTION("All operator tokens")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("+ - * / ^ %");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TtType::Plus);
    REQUIRE(tokens[1].type == TtType::Minus);
    REQUIRE(tokens[2].type == TtType::Star);
    REQUIRE(tokens[3].type == TtType::Slash);
    REQUIRE(tokens[4].type == TtType::Caret);
    REQUIRE(tokens[5].type == TtType::Percent);
  }

  SECTION("Decimal starting with dot")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize(".345");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type == TtType::Number);
    REQUIRE(tokens[0].text == ".345");
  }

  SECTION("Complex expression")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("sin(pi / 2)");
    REQUIRE(tokens.size() == 6);
    REQUIRE(tokens[0].type == TtType::Identifier);
    REQUIRE(tokens[0].text == "sin");
    REQUIRE(tokens[1].type == TtType::LParen);
    REQUIRE(tokens[2].type == TtType::Identifier);
    REQUIRE(tokens[2].text == "pi");
    REQUIRE(tokens[3].type == TtType::Slash);
    REQUIRE(tokens[4].type == TtType::Number);
    REQUIRE(tokens[4].text == "2");
    REQUIRE(tokens[5].type == TtType::RParen);
  }

  SECTION("Negative number tokenizes as minus + number")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("-3.14");
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == TtType::Minus);
    REQUIRE(tokens[1].type == TtType::Number);
    REQUIRE(tokens[1].text == "3.14");
  }

  SECTION("Empty string")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("");
    REQUIRE(tokens.empty());
  }

  SECTION("Parentheses")
  {
    auto tokens = nx::core::ArrayCalculatorParser::Tokenize("(3+4)");
    REQUIRE(tokens.size() == 5);
    REQUIRE(tokens[0].type == TtType::LParen);
    REQUIRE(tokens[4].type == TtType::RParen);
  }
}

// -----------------------------------------------------------------------------
// Test 1: Array Resolution
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Array Resolution")
{
  unit_test::LoadPlugins();

  // Create a DataStructure with arrays in multiple groups.
  // "Group1/SharedName" (UInt32, 10 tuples, filled with 5)
  // "Group2/SharedName" (UInt32, 10 tuples, filled with 7)
  // "Group1/UniqueName" (UInt32, 10 tuples, filled with 3)
  DataStructure dataStructure;
  AttributeMatrix* group1Ptr = AttributeMatrix::Create(dataStructure, "Group1", {10ULL});
  auto group1Id = group1Ptr->getId();
  AttributeMatrix* group2Ptr = AttributeMatrix::Create(dataStructure, "Group2", {10ULL});
  auto group2Id = group2Ptr->getId();

  UInt32ArrayType* shared1Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, "SharedName", {10}, {1}, group1Id);
  shared1Ptr->fill(5);
  UInt32ArrayType* shared2Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, "SharedName", {10}, {1}, group2Id);
  shared2Ptr->fill(7);
  UInt32ArrayType* unique1Ptr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, "UniqueName", {10}, {1}, group1Id);
  unique1Ptr->fill(3);

  ArrayCalculatorFilter filter;

  SECTION("Unique bare name resolves without selected group")
  {
    DataPath outputPath({"Group1", "NewArray"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = DataPath{}, .equation = "UniqueName + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(outputPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 4.0, 0.01));
    }
  }

  SECTION("Ambiguous bare name with no selected group gives error")
  {
    DataPath outputPath({"Group1", "AmbiguousResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = DataPath{}, .equation = "SharedName + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::AmbiguousArrayName));
  }

  SECTION("Selected group resolves ambiguity")
  {
    DataPath outputPath({"Group1", "ResolvedResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = DataPath({"Group1"}), .equation = "SharedName + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(outputPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 6.0, 0.01));
    }
  }

  SECTION("Quoted full path resolves directly")
  {
    DataPath outputPath({"Group2", "QuotedResult"});
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = DataPath{}, .equation = "\"Group2/SharedName\" + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(outputPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(outputPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 8.0, 0.01));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Test 2: Built-in Constants
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Built-in Constants")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("pi resolves to std::numbers::pi")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "pi", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), std::numbers::pi, 0.0001));
    }
  }

  SECTION("e resolves to std::numbers::e")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "e", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), std::numbers::e, 0.0001));
    }
  }

  SECTION("2 * pi expression works")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "2 * pi", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 2.0 * std::numbers::pi, 0.0001));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Test 3: Modulo Operator
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Modulo Operator")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("Scalar modulo: 10 % 3 = 1")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "10 % 3", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 1.0, 0.01));
    }
  }

  SECTION("Array modulo: InputArray2 % 3 = 1 element-wise")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "InputArray2 % 3", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 1.0, 0.01));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Test 4: Tuple+Component Indexing
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Tuple Component Indexing")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("MultiComponent Array1[2, 1] produces scalar value 7")
  {
    // MultiComponent Array1 has 10 tuples x 3 comps, values 0..29 sequentially.
    // tuple 2, comp 1 = index 2*3 + 1 = 7
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(
                            CalculatorParameter::ValueType{.m_selected_group = k_AttributeMatrixPath, .equation = "\"MultiComponent Array1\"[2, 1]", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    // Scalar result broadcast to all tuples in the AttributeMatrix (10 tuples)
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 7.0, 0.01));
    }
  }

  SECTION("MultiComponent Array1[100, 0] out of bounds gives TupleOutOfRange error")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(
                            CalculatorParameter::ValueType{.m_selected_group = k_AttributeMatrixPath, .equation = "\"MultiComponent Array1\"[100, 0]", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::TupleOutOfRange));
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Test 5: Sub-expression Component Access
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Sub-expression Component Access")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("(MultiComponent Array1 + MultiComponent Array2)[0] extracts component 0 of the sum")
  {
    // MultiComponent Array1 and Array2 both have 10 tuples x 3 components, values 0..29.
    // Sum = 2*values = [0,2,4,6,8,10,...,58]
    // Component 0 extraction: for each tuple t, take element at (t*3 + 0) = 2*(t*3)
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                            .m_selected_group = k_AttributeMatrixPath, .equation = R"(("MultiComponent Array1" + "MultiComponent Array2")[0])", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    REQUIRE(outputArray.getNumberOfComponents() == 1);
    for(usize t = 0; t < 10; t++)
    {
      // Component 0 of sum: (t*3 + 0) + (t*3 + 0) = 2 * (t * 3)
      double expected = 2.0 * static_cast<double>(t * 3);
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(t), expected, 0.01));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Test 6: Multi-word Array Names
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Multi-word Array Names")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  SECTION("Spaced Array + 1 = 3")
  {
    // Spaced Array is filled with 2, so Spaced Array + 1 = 3
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = k_AttributeMatrixPath, .equation = "Spaced Array + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    REQUIRE(outputArray.getNumberOfTuples() == 10);
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), 3.0, 0.01));
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Sub-expression Tuple Component Extraction")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;

  // MultiComponent Array1 has 10 tuples, 3 components, values 0,1,2,3,...,29
  // (ArrayA + ArrayB) at tuple 2, component 1 = 2*(2*3+1) = 14

  SECTION("(expr)[T, C] produces scalar")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                            .m_selected_group = k_AttributeMatrixPath, .equation = R"(("MultiComponent Array1" + "MultiComponent Array2")[2, 1])", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
    const auto& outputArray = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
    // Scalar result broadcast to AM shape (10 tuples)
    double expected = 2.0 * (2 * 3 + 1); // tuple 2, comp 1, doubled = 14
    for(usize i = 0; i < outputArray.getNumberOfTuples(); i++)
    {
      REQUIRE(unit_test::CloseEnough<double>(outputArray.at(i), expected, 0.01));
    }
  }

  SECTION("(expr)[T, C] out of bounds tuple")
  {
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                            .m_selected_group = k_AttributeMatrixPath, .equation = R"(("MultiComponent Array1" + "MultiComponent Array2")[100, 0])", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == static_cast<int32>(CalculatorErrorCode::TupleOutOfRange));
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Preflight rejects missing binary function operands", "[SimplnxCore][ArrayCalculatorFilter]")
{
  unit_test::LoadPlugins();
  const std::string equation = GENERATE("root(1,)", "root(,1)", "log(1,)", "log(,1)", "min(1,)", "min(,1)", "max(1,)", "max(,1)");
  CAPTURE(equation);
  DataStructure dataStructure = ::CreateDataStructure();
  ArrayCalculatorFilter filter;
  Arguments args;
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                            .m_selected_group = k_AttributeMatrixPath, .equation = equation, .m_units = CalculatorParameter::Radians}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));

  REQUIRE_FALSE(dataStructure.containsData(k_AttributeArrayPath));
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  REQUIRE(preflightResult.output_actions.errors().front().code == static_cast<int32>(CalculatorErrorCode::InvalidEquation));
  REQUIRE_FALSE(dataStructure.containsData(k_AttributeArrayPath));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Component shape mismatch after extraction", "[SimplnxCore][ArrayCalculatorFilter][ComponentShapeRegression]")
{
  unit_test::LoadPlugins();
  const std::string equation = GENERATE("A + (B)[0]", "(B)[0] + A");
  CAPTURE(equation);
  DataStructure dataStructure = CreateComponentShapeDataStructure();
  ArrayCalculatorFilter filter;
  Arguments args;
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                            .m_selected_group = k_AttributeMatrixPath, .equation = equation, .m_units = CalculatorParameter::Radians}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(k_AttributeArrayPath));

  // Component extraction leaves an array with one component, not a broadcast scalar.
  // Its component count differs from A's three components in either operand order.
  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE(result.output_actions.errors().size() == 1);
  REQUIRE(result.output_actions.errors().front().code == static_cast<int32>(CalculatorErrorCode::InconsistentCompDims));
  REQUIRE_FALSE(dataStructure.containsData(k_AttributeArrayPath));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Component shape and scalar broadcast controls", "[SimplnxCore][ArrayCalculatorFilter][ComponentShapeRegression]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure = CreateComponentShapeDataStructure();
  ArrayCalculatorFilter filter;
  std::string equation;
  std::vector<float64> expected;
  // Hand calculation for A = (1, 2, 4) and B = (10, 20, 40).
  SECTION("Matching three-component arrays")
  {
    equation = "A + B";
    expected = {11, 22, 44};
  }
  SECTION("Matching extracted components")
  {
    equation = "(A)[1] + (B)[0]";
    expected = {12};
  }
  SECTION("Scalar on the right")
  {
    equation = "A + 5";
    expected = {6, 7, 9};
  }
  SECTION("Scalar on the left")
  {
    equation = "5 + A";
    expected = {6, 7, 9};
  }

  const auto result = CreateAndExecuteArrayCalculatorFilter(equation, k_AttributeArrayPath, CalculatorParameter::Radians, dataStructure, filter);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath));
  const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(k_AttributeArrayPath);
  REQUIRE(output.getNumberOfTuples() == 1);
  REQUIRE(output.getNumberOfComponents() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(output[i] == expected[i]);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Bulk multi-page calculation and failure propagation", "[SimplnxCore][ArrayCalculatorFilter]")
{
  unit_test::LoadPlugins();
  constexpr usize k_TupleCount = 21847;
  constexpr usize k_ComponentCount = 3;
  const DataPath matrixPath({"BulkMatrix"});
  const DataPath sourcePath = matrixPath.createChildPath("Input");
  const DataPath outputPath = matrixPath.createChildPath("Calculated");

  SECTION("indexed scalar broadcast crosses the final partial page")
  {
    const auto [equation, expectedOffset] =
        GENERATE(std::make_pair("Input + Input[1, 1]", uint32{6}), std::make_pair("Input[1, 1] + Input", uint32{6}), std::make_pair("Input + (Input + Input)[1, 1]", uint32{12}));
    CAPTURE(equation);
    DataStructure dataStructure;
    auto* matrixPtr = AttributeMatrix::Create(dataStructure, matrixPath.getTargetName(), ShapeType{k_TupleCount});
    REQUIRE(matrixPtr != nullptr);
    auto* sourcePtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, sourcePath.getTargetName(), matrixPtr->getShape(), ShapeType{k_ComponentCount}, matrixPtr->getId());
    REQUIRE(sourcePtr != nullptr);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      for(usize compIdx = 0; compIdx < k_ComponentCount; compIdx++)
      {
        (*sourcePtr)[tupleIdx * k_ComponentCount + compIdx] = static_cast<int32>(tupleIdx * 5 + compIdx);
      }
    }

    ArrayCalculatorFilter filter;
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key,
                        std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{.m_selected_group = matrixPath, .equation = equation, .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::uint32));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(outputPath));
    const auto& output = dataStructure.getDataRefAs<UInt32ArrayType>(outputPath);
    REQUIRE(output.getNumberOfTuples() == k_TupleCount);
    REQUIRE(output.getNumberOfComponents() == k_ComponentCount);
    for(usize tupleIdx = 0; tupleIdx < k_TupleCount; tupleIdx++)
    {
      for(usize compIdx = 0; compIdx < k_ComponentCount; compIdx++)
      {
        const auto expected = static_cast<uint32>(tupleIdx * 5 + compIdx + expectedOffset);
        REQUIRE(output[tupleIdx * k_ComponentCount + compIdx] == expected);
      }
    }
    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("later source page failure stops before the tail write")
  {
    constexpr int32 k_ReadError = -91901;
    DataStructure dataStructure;
    auto* matrixPtr = AttributeMatrix::Create(dataStructure, matrixPath.getTargetName(), ShapeType{k_TupleCount});
    REQUIRE(matrixPtr != nullptr);
    auto sourceStore = std::make_shared<ArrayCalculatorFailOnLaterReadStore<int32>>(ShapeType{k_TupleCount}, ShapeType{k_ComponentCount}, int32{0}, k_ReadError, 2);
    auto* sourcePtr = Int32ArrayType::Create(dataStructure, sourcePath.getTargetName(), sourceStore, matrixPtr->getId());
    REQUIRE(sourcePtr != nullptr);
    for(usize valueIdx = 0; valueIdx < sourcePtr->getSize(); valueIdx++)
    {
      (*sourcePtr)[valueIdx] = static_cast<int32>(valueIdx);
    }

    ArrayCalculatorFilter filter;
    Arguments args;
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                              .m_selected_group = matrixPath, .equation = "Input + 1", .m_units = CalculatorParameter::Radians}));
    args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::uint32));
    args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(std::any_of(executeResult.result.errors().cbegin(), executeResult.result.errors().cend(), [](const Error& error) { return error.code == k_ReadError; }));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(outputPath));
    const auto& output = dataStructure.getDataRefAs<UInt32ArrayType>(outputPath);
    REQUIRE(output[0] == 1U);
    REQUIRE(output[output.getSize() - 1] == 0U);
  }
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ArrayCalculatorFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ArrayCalculatorFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ArrayCalculatorFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ArrayCalculatorFilter>::uuid);

      // Note: Complex SIMPL parameter conversions may produce warnings
      // pipelineFilter->getComments() may not be empty for filters with custom converters

      const Arguments args = pipelineFilterPtr->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        // Complex type (NumericTypeParameterConverter) - verified by successful pipeline loading
      }
      // Complex type (DataArrayCreationFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

TEST_CASE("SimplnxCore::ArrayCalculatorFilter: Function result indexing", "[SimplnxCore][ArrayCalculatorFilter]")
{
  DataStructure dataStructure;
  auto* matrixPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrix, {2});
  REQUIRE(matrixPtr != nullptr);
  auto* firstPtr = Float64ArrayType::CreateWithStore<Float64DataStoreType>(dataStructure, "MC1", {2}, {3}, matrixPtr->getId());
  auto* secondPtr = Float64ArrayType::CreateWithStore<Float64DataStoreType>(dataStructure, "MC2", {2}, {3}, matrixPtr->getId());
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  const std::vector<float64> firstValues = {1, -8, 3, 9, 5, 2};
  const std::vector<float64> secondValues = {4, 6, 2, 7, -3, 10};
  for(usize i = 0; i < firstValues.size(); i++)
  {
    (*firstPtr)[i] = firstValues[i];
    (*secondPtr)[i] = secondValues[i];
  }

  std::string equation;
  std::vector<float64> expected;
  bool expectInvalid = false;
  // Literal, hand-calculated results for two tuples with three components each.
  SECTION("Maximum result component")
  {
    equation = R"(max("MC1", "MC2")[1])";
    expected = {6, 5};
  }
  SECTION("Minimum result component")
  {
    equation = R"(min("MC1", "MC2")[0])";
    expected = {1, 7};
  }
  SECTION("Maximum result tuple and component")
  {
    equation = R"(max("MC1", "MC2")[1, 2])";
    expected = {10};
  }
  SECTION("Array argument indexing remains a shape error")
  {
    equation = R"(max("MC1", "MC2"[1]))";
    expectInvalid = true;
  }
  SECTION("Parenthesized sum component")
  {
    equation = R"(("MC1" + "MC2")[1])";
    expected = {-2, 2};
  }
  SECTION("Unary function result component")
  {
    equation = R"(abs("MC1")[1])";
    expected = {8, 5};
  }
  SECTION("Unary function result tuple and component")
  {
    equation = R"(abs("MC1")[0, 1])";
    expected = {8};
  }
  SECTION("Nested function result component")
  {
    equation = R"(max(abs("MC1"), "MC2")[1])";
    expected = {8, 5};
  }
  CAPTURE(equation);
  ArrayCalculatorFilter filter;
  Arguments args;
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatorParameter_Key, std::make_any<CalculatorParameter::ValueType>(CalculatorParameter::ValueType{
                                                                            .m_selected_group = k_AttributeMatrixPath, .equation = equation, .m_units = CalculatorParameter::Radians}));
  args.insertOrAssign(ArrayCalculatorFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(NumericType::float64));
  const DataPath outputPath({"Result"});
  args.insertOrAssign(ArrayCalculatorFilter::k_CalculatedArray_Key, std::make_any<DataPath>(outputPath));
  const auto preflight = filter.preflight(dataStructure, args);
  if(expectInvalid)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE(preflight.output_actions.errors().front().code == static_cast<int32>(CalculatorErrorCode::InconsistentCompDims));
    unit_test::CheckArraysInheritTupleDims(dataStructure);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  DataStructure preview = dataStructure;
  auto actions = preflight.output_actions.value();
  const auto applied = actions.applyAll(preview, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
  REQUIRE_NOTHROW(preview.getDataRefAs<Float64ArrayType>(outputPath));
  const auto& planned = preview.getDataRefAs<Float64ArrayType>(outputPath);
  REQUIRE(planned.getTupleShape() == ShapeType{expected.size()});
  REQUIRE(planned.getComponentShape() == ShapeType{1});
  const auto executed = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executed.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(outputPath);
  REQUIRE(output.getTupleShape() == planned.getTupleShape());
  REQUIRE(output.getComponentShape() == planned.getComponentShape());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(output[i] == expected[i]);
  }
  unit_test::CheckArraysInheritTupleDims(preview);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
