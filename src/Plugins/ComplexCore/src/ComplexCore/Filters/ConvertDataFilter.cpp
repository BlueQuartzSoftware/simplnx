#include "ConvertDataFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ConvertData.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertDataFilter::name() const
{
  return FilterTraits<ConvertDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertDataFilter::className() const
{
  return FilterTraits<ConvertDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertDataFilter::uuid() const
{
  return FilterTraits<ConvertDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertDataFilter::humanName() const
{
  return "Convert AttributeArray DataType";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertDataFilter::defaultTags() const
{
  return {"Core", "Convert"};
}

//------------------------------------------------------------------------------
Parameters ConvertDataFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_ScalarType_Key, "Scalar Type", "Convert to this data type", 0, GetAllDataTypesAsStrings()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayToConvert_Key, "Data Array to Convert", "The complete path to the Data Array to Convert", DataPath{}, GetAllDataTypes()));
  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ConvertedArray_Key, "Converted Data Array", "The name of the converted Data Array", "Converted_"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertDataFilter::clone() const
{
  return std::make_unique<ConvertDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pScalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  auto pInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvert_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArray_Key);
  DataPath convertedArrayPath = pInputArrayPath.getParent().createChildPath(pConvertedArrayName);

  DataType pScalarType = StringToDataType(GetAllDataTypesAsStrings()[pScalarTypeIndex]);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;

  auto* inputArray = dataStructure.getDataAs<IDataArray>(pInputArrayPath);
  if(inputArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-15201, fmt::format("Cannot find input data array at path '{}'", pInputArrayPath.toString())}})};
  }

  resultOutputActions.value().appendAction(
      std::make_unique<CreateArrayAction>(pScalarType, inputArray->getIDataStoreRef().getTupleShape(), inputArray->getIDataStoreRef().getComponentShape(), convertedArrayPath));

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  ConvertDataInputValues inputValues;
  uint64 scalarTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ScalarType_Key);
  inputValues.ScalarType = StringToDataType(GetAllDataTypesAsStrings()[scalarTypeIndex]);
  inputValues.SelectedArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_ArrayToConvert_Key);
  auto pConvertedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ConvertedArray_Key);
  inputValues.OutputArrayName = inputValues.SelectedArrayPath.getParent().createChildPath(pConvertedArrayName);

  return ConvertData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
