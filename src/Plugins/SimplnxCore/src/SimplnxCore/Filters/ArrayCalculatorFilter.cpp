#include "ArrayCalculatorFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ArrayCalculator.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<CalculatorParameter>(k_CalculatorParameter_Key, "Infix Expression", "The mathematical expression used to calculate the output array", CalculatorParameter::ValueType{}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Output Numeric Type", "The data type of the calculated array", NumericType::float64));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CalculatedArray_Key, "Output Calculated Array", "The path to the calculated array", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ArrayCalculatorFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ArrayCalculatorFilter::clone() const
{
  return std::make_unique<ArrayCalculatorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ArrayCalculatorFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                              const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pInfixEquationValue = filterArgs.value<CalculatorParameter::ValueType>(k_CalculatorParameter_Key);
  auto pScalarTypeValue = filterArgs.value<NumericTypeParameter::ValueType>(k_ScalarType_Key);
  auto pCalculatedArrayPath = filterArgs.value<DataPath>(k_CalculatedArray_Key);
  auto outputGroupPath = pCalculatedArrayPath.getParent();

  nx::core::Result<OutputActions> resultOutputActions;

  // Parse and validate the expression
  const std::atomic_bool mShouldCancel(false);
  ArrayCalculatorParser parser(dataStructure, pInfixEquationValue.m_selected_group, pInfixEquationValue.equation, mShouldCancel);
  std::vector<usize> calculatedTupleShape;
  std::vector<usize> calculatedComponentShape;
  Result<> parseResult = parser.parseAndValidate(calculatedTupleShape, calculatedComponentShape);

  // Transfer warnings
  resultOutputActions.warnings() = parseResult.warnings();

  if(parseResult.invalid())
  {
    return {nonstd::make_unexpected(parseResult.errors())};
  }

  // If the result is a scalar (1 tuple) and the output is in an AttributeMatrix,
  // use the AttributeMatrix's shape instead
  if(calculatedTupleShape.size() == 1 && calculatedTupleShape[0] == 1)
  {
    if(const auto* attributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(outputGroupPath); attributeMatrixPtr != nullptr)
    {
      calculatedTupleShape = attributeMatrixPtr->getShape();
    }
  }

  // Create the output array
  auto createArrayAction = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), calculatedTupleShape, calculatedComponentShape, pCalculatedArrayPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ArrayCalculatorFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ArrayCalculatorInputValues inputValues;
  auto pInfixEquationValue = filterArgs.value<CalculatorParameter::ValueType>(k_CalculatorParameter_Key);
  inputValues.infix_equation = pInfixEquationValue.equation;
  inputValues.units = pInfixEquationValue.m_units;
  inputValues.selected_group = pInfixEquationValue.m_selected_group;
  inputValues.scalar_type = filterArgs.value<NumericTypeParameter::ValueType>(k_ScalarType_Key);
  inputValues.calculated_array = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  return ArrayCalculator(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SelectedAttributeMatrixKey = "SelectedAttributeMatrix";
constexpr StringLiteralType k_InfixEquationKey = "InfixEquation";
constexpr StringLiteralType k_ScalarTypeKey = "ScalarType";
constexpr StringLiteralType k_CalculatedArrayKey = "CalculatedArray";
} // namespace simpl
} // namespace

Result<Arguments> ArrayCalculatorFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ArrayCalculatorFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedAttributeMatrixKey,
  // "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(simpl_conversion::ConvertTopParameters<simpl_conversion::CalculatorFilterParameterConverter>(args, json, k_CalculatorParameter_Key));
  Result<> result = simpl_conversion::ConvertParameter<simpl_conversion::NumericTypeParameterConverter>(args, json, simpl::k_ScalarTypeKey, k_ScalarType_Key);
  if(result.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(result));
  }
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationFilterParameterConverter>(args, json, simpl::k_CalculatedArrayKey, k_CalculatedArray_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
