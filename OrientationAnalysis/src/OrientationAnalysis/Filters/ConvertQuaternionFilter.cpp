#include "ConvertQuaternionFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertQuaternion.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

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
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertQuaternionFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, k_Choices));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Input Quaternions", "Specifies the quaternions to convert", DataPath({"CellData", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Quaternions", "", DataPath({"Quaternions [Converted]"})));

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
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Validate the Quats array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pQuaternionDataArrayPathValue);
  if(quats.getNumberOfComponents() != 4)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion Input Array must be a 4 component Float32 array"}})};
  }

  {
    auto createConvertedQuatAction = std::make_unique<CreateArrayAction>(DataType::float32, quats.getTupleShape(), std::vector<usize>{4}, pOutputDataArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createConvertedQuatAction));
  }

  auto pRemoveOriginalArray = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  if(pRemoveOriginalArray)
  {
    resultOutputActions.value().deferredActions.push_back(std::make_unique<DeleteDataAction>(pQuaternionDataArrayPathValue));
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
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);
  inputValues.ConversionType = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  return ConvertQuaternion(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
