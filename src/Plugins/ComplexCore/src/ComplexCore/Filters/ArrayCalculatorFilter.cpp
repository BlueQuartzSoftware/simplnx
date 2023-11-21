#include "ArrayCalculatorFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ArrayCalculator.hpp"
#include "ComplexCore/utils/ICalculatorArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/CalculatorParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ArrayCalculatorFilter::name() const
{
  return FilterTraits<ArrayCalculatorFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ArrayCalculatorFilter::className() const
{
  return FilterTraits<ArrayCalculatorFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ArrayCalculatorFilter::uuid() const
{
  return FilterTraits<ArrayCalculatorFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ArrayCalculatorFilter::humanName() const
{
  return "Attribute Array Calculator";
}

//------------------------------------------------------------------------------
std::vector<std::string> ArrayCalculatorFilter::defaultTags() const
{
  return {className(), "Core", "Generation"};
}

//------------------------------------------------------------------------------
Parameters ArrayCalculatorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<CalculatorParameter>(k_CalculatorParameter_Key, "Infix Expression", "The mathematical expression used to calculate the output array", CalculatorParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Output Numeric Type", "The data type of the calculated array", NumericType::float64));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CalculatedArray_Key, "Output Calculated Array", "The path to the calculated array", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ArrayCalculatorFilter::clone() const
{
  return std::make_unique<ArrayCalculatorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ArrayCalculatorFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pInfixEquationValue = filterArgs.value<CalculatorParameter::ValueType>(k_CalculatorParameter_Key);
  auto pScalarTypeValue = filterArgs.value<NumericTypeParameter::ValueType>(k_ScalarType_Key);
  auto pCalculatedArrayPath = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  auto pSelectedGroupPath = pInfixEquationValue.m_SelectedGroup;
  auto outputGroupPath = pCalculatedArrayPath.getParent();

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // parse the infix expression
  ArrayCalculatorParser parser(dataStructure, pSelectedGroupPath, pInfixEquationValue.m_Equation, true);
  std::vector<CalculatorItem::Pointer> parsedInfix;
  Result<> parsedEquationResults = parser.parseInfixEquation(parsedInfix);
  resultOutputActions.warnings() = parsedEquationResults.warnings();
  if(parsedEquationResults.invalid())
  {
    return {nonstd::make_unexpected(parsedEquationResults.errors())};
  }
  if(parsedInfix.empty())
  {
    return MakePreflightErrorResult(-7760, "Error while parsing infix expression.");
  }

  // check individual infix expression items for validity
  for(int i = 0; i < parsedInfix.size(); i++)
  {
    CalculatorItem::Pointer calcItem = parsedInfix[i];
    std::string errMsg = "";
    CalculatorItem::ErrorCode err = calcItem->checkValidity(parsedInfix, i, errMsg);
    int errInt = static_cast<int>(err);
    if(errInt < 0)
    {
      return MakePreflightErrorResult(errInt, errMsg);
    }
  }

  // collect calculated array dimensions, check for consistent array component dimensions in infix expression & make sure it yields a numeric result
  std::vector<usize> calculatedTupleShape;
  std::vector<usize> calculatedComponentShape;
  ICalculatorArray::ValueType resultType = ICalculatorArray::ValueType::Unknown;
  for(const auto& item1 : parsedInfix)
  {
    if(item1->isICalculatorArray())
    {
      ICalculatorArray::Pointer array1 = std::dynamic_pointer_cast<ICalculatorArray>(item1);
      if(item1->isArray())
      {
        if(!calculatedComponentShape.empty() && resultType == ICalculatorArray::ValueType::Array && calculatedComponentShape != array1->getArray()->getComponentShape())
        {
          return MakePreflightErrorResult(static_cast<int>(CalculatorItem::ErrorCode::InconsistentCompDims), "Attribute Array symbols in the infix expression have mismatching component dimensions");
        }
        if(!calculatedTupleShape.empty() && resultType == ICalculatorArray::ValueType::Array && calculatedTupleShape[0] != array1->getArray()->getNumberOfTuples())
        {
          return MakePreflightErrorResult(static_cast<int>(CalculatorItem::ErrorCode::InconsistentTuples), "Attribute Array symbols in the infix expression have mismatching number of tuples");
        }

        resultType = ICalculatorArray::ValueType::Array;
        calculatedComponentShape = array1->getArray()->getComponentShape();
        calculatedTupleShape = {array1->getArray()->getNumberOfTuples()};
      }
      else if(resultType == ICalculatorArray::ValueType::Unknown)
      {
        resultType = ICalculatorArray::ValueType::Number;
        calculatedComponentShape = array1->getArray()->getComponentShape();
        calculatedTupleShape = {array1->getArray()->getNumberOfTuples()};
      }
    }
  }
  if(resultType == ICalculatorArray::ValueType::Unknown)
  {
    return MakePreflightErrorResult(static_cast<int>(CalculatorItem::ErrorCode::NoNumericArguments), "The expression does not have any arguments that simplify down to a number.");
  }

  if(resultType == ICalculatorArray::ValueType::Number)
  {
    if(const auto* attributeMatrix = dataStructure.getDataAs<AttributeMatrix>(outputGroupPath); attributeMatrix != nullptr)
    {
      calculatedTupleShape = attributeMatrix->getShape();
      resultOutputActions.warnings().push_back(Warning{static_cast<int>(CalculatorItem::WarningCode::NumericValueWarning),
                                                       "The result of the chosen expression will be a numeric value. This numeric value will be used to initialize an "
                                                       "array with the number of tuples equal to the number of tuples in the destination group."});
    }
    else
    {
      resultOutputActions.warnings().push_back(
          Warning{static_cast<int>(CalculatorItem::WarningCode::NumericValueWarning),
                  "The result of the chosen expression will be a numeric value or contain one tuple. This numeric value will be stored in an array with the number of tuples equal to 1"});
    }
  }

  // convert to postfix notation
  Result<ArrayCalculatorParser::ParsedEquation> rpnResults = ArrayCalculatorParser::ToRPN(pInfixEquationValue.m_Equation, parsedInfix);
  std::vector<CalculatorItem::Pointer> rpn = rpnResults.value();
  if(rpnResults.invalid() || rpn.empty())
  {
    return MakePreflightErrorResult(-7761, "Error while converting parsed infix expression to postfix notation");
  }

  // create the destination array for the calculated results
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), calculatedTupleShape, calculatedComponentShape, pCalculatedArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
} // namespace complex

//------------------------------------------------------------------------------
Result<> ArrayCalculatorFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{

  ArrayCalculatorInputValues inputValues;
  auto pInfixEquationValue = filterArgs.value<CalculatorParameter::ValueType>(k_CalculatorParameter_Key);
  inputValues.InfixEquation = pInfixEquationValue.m_Equation;
  inputValues.Units = pInfixEquationValue.m_Units;
  inputValues.SelectedGroup = pInfixEquationValue.m_SelectedGroup;
  inputValues.ScalarType = filterArgs.value<NumericTypeParameter::ValueType>(k_ScalarType_Key);
  inputValues.CalculatedArray = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  return ArrayCalculator(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
