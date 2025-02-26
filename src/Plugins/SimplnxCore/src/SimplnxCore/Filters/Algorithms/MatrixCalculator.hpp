#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT MatrixCalculatorInputValues
{
  ChoicesParameter::ValueType Operation;
  MultiArraySelectionParameter::ValueType SelectedPaths;
  ArrayCreationParameter::ValueType OutputPath;
};

namespace matrix_calculator::constants
{

const std::string k_Addition("+ (Addition)");
const std::string k_Subtraction("- (Subtraction)");
const std::string k_Multiplication("* (Multiplication)");
const nx::core::ChoicesParameter::Choices k_OperationChoices = {k_Addition, k_Subtraction, k_Multiplication};

const nx::core::ChoicesParameter::ValueType k_AdditionIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_SubtractionIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_MultiplicationIdx = 2ULL;
} // namespace matrix_calculator::constants

/**
 * @class
 */
class SIMPLNXCORE_EXPORT MatrixCalculator
{
public:
  MatrixCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MatrixCalculatorInputValues* inputValues);
  ~MatrixCalculator() noexcept;

  MatrixCalculator(const MatrixCalculator&) = delete;
  MatrixCalculator(MatrixCalculator&&) noexcept = delete;
  MatrixCalculator& operator=(const MatrixCalculator&) = delete;
  MatrixCalculator& operator=(MatrixCalculator&&) noexcept = delete;

  // Error Codes
  enum class ErrorCodes : int32
  {
    EmptyInputArrays = -2350,
    OneInputArray = -2351,
    NonPositiveTupleDimValue = -2352,
    TypeNameMismatch = -2353,
    ComponentShapeMismatch = -2354,
    InputArraysEqualAny = -2355,
    InputArraysUnsupported = -2356
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MatrixCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
