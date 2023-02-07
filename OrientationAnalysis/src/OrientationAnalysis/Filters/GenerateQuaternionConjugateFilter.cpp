#include "GenerateQuaternionConjugateFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GenerateQuaternionConjugate.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace
{
inline constexpr int32 k_IncorrectInputArray = -7100;
inline constexpr int32 k_MissingInputArray = -7101;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateQuaternionConjugateFilter::name() const
{
  return FilterTraits<GenerateQuaternionConjugateFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateQuaternionConjugateFilter::className() const
{
  return FilterTraits<GenerateQuaternionConjugateFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateQuaternionConjugateFilter::uuid() const
{
  return FilterTraits<GenerateQuaternionConjugateFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateQuaternionConjugateFilter::humanName() const
{
  return "Generate Quaternion Conjugate";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateQuaternionConjugateFilter::defaultTags() const
{
  return {"Processing", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateQuaternionConjugateFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Quaternions", "Specifies the quaternions to convert", DataPath({"CellData", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath({"Quaternions [Conjugate]"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateQuaternionConjugateFilter::clone() const
{
  return std::make_unique<GenerateQuaternionConjugateFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateQuaternionConjugateFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{

  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

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
Result<> GenerateQuaternionConjugateFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  GenerateQuaternionConjugateInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return GenerateQuaternionConjugate(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
