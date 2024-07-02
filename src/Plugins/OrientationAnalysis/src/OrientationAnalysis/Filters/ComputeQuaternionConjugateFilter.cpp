#include "ComputeQuaternionConjugateFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeQuaternionConjugate.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace
{
 constexpr int32 k_IncorrectInputArray = -7100;
 constexpr int32 k_MissingInputArray = -7101;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeQuaternionConjugateFilter::name() const
{
  return FilterTraits<ComputeQuaternionConjugateFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeQuaternionConjugateFilter::className() const
{
  return FilterTraits<ComputeQuaternionConjugateFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeQuaternionConjugateFilter::uuid() const
{
  return FilterTraits<ComputeQuaternionConjugateFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeQuaternionConjugateFilter::humanName() const
{
  return "Compute Quaternion Conjugate";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeQuaternionConjugateFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography", "Orientation", "Quaternion"};
}

//------------------------------------------------------------------------------
Parameters ComputeQuaternionConjugateFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "Should the original Data be deleted from the DataStructure", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Quaternions", "Specifies the quaternions to convert", DataPath({"Cell Data", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputDataArrayName_Key, "Created Quaternion Conjugate", "The name of the generated quaternion conjugate array", "Quaternions [Conjugate]"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeQuaternionConjugateFilter::clone() const
{
  return std::make_unique<ComputeQuaternionConjugateFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeQuaternionConjugateFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pOutputDataArrayPathValue = pQuaternionDataArrayPathValue.replaceName(filterArgs.value<std::string>(k_OutputDataArrayName_Key));

  nx::core::Result<OutputActions> resultOutputActions;
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
Result<> ComputeQuaternionConjugateFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  ComputeQuaternionConjugateInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.OutputDataArrayPath = inputValues.QuaternionDataArrayPath.replaceName(filterArgs.value<std::string>(k_OutputDataArrayName_Key));
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return ComputeQuaternionConjugate(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_QuaternionDataArrayPathKey = "QuaternionDataArrayPath";
constexpr StringLiteral k_OutputDataArrayPathKey = "OutputDataArrayPath";
constexpr StringLiteral k_DeleteOriginalDataKey = "DeleteOriginalData";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeQuaternionConjugateFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeQuaternionConjugateFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuaternionDataArrayPathKey, k_CellQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_OutputDataArrayPathKey, k_OutputDataArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_DeleteOriginalDataKey, k_DeleteOriginalData_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
