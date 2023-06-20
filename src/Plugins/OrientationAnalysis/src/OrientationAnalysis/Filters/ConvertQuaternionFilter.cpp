#include "ConvertQuaternionFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertQuaternion.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace
{
const ChoicesParameter::Choices k_Choices = {"To Scalar Vector ( w, [x, y, z] )", "To Vector Scalar ( [x, y, z], w )"};

inline constexpr int32 k_IncorrectInputArray = -7000;
inline constexpr int32 k_MissingInputArray = -7001;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertQuaternionFilter::name() const
{
  return FilterTraits<ConvertQuaternionFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertQuaternionFilter::className() const
{
  return FilterTraits<ConvertQuaternionFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertQuaternionFilter::uuid() const
{
  return FilterTraits<ConvertQuaternionFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertQuaternionFilter::humanName() const
{
  return "Convert Quaternion Order";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertQuaternionFilter::defaultTags() const
{
  return {"Processing", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertQuaternionFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, k_Choices));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Input Quaternions", "Specifies the quaternions to convert", DataPath({"CellData", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputDataArrayPath_Key, "Output Quaternions", "", "Quaternions [Converted]"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertQuaternionFilter::clone() const
{
  return std::make_unique<ConvertQuaternionFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertQuaternionFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pOutputDataArrayPathValue = pQuaternionDataArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Validate the Quats array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pQuaternionDataArrayPathValue);
  if(quats.getNumberOfComponents() != 4)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion Input Array must be a 4 component Float32 array"}})};
  }

  {
    auto createConvertedQuatAction = std::make_unique<CreateArrayAction>(DataType::float32, quats.getTupleShape(), std::vector<usize>{4}, pOutputDataArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createConvertedQuatAction));
  }

  auto pRemoveOriginalArray = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  if(pRemoveOriginalArray)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pQuaternionDataArrayPathValue));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertQuaternionFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  ConvertQuaternionInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.OutputDataArrayPath = inputValues.QuaternionDataArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);
  inputValues.ConversionType = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  return ConvertQuaternion(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
