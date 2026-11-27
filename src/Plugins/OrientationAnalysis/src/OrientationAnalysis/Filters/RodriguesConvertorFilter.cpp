#include "RodriguesConvertorFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/RodriguesConvertor.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::name() const
{
  return FilterTraits<RodriguesConvertorFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::className() const
{
  return FilterTraits<RodriguesConvertorFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RodriguesConvertorFilter::uuid() const
{
  return FilterTraits<RodriguesConvertorFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::humanName() const
{
  return "Rodrigues Convertor";
}

//------------------------------------------------------------------------------
std::vector<std::string> RodriguesConvertorFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography", "Convert"};
}

//------------------------------------------------------------------------------
Parameters RodriguesConvertorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "Should the original Rodrigues data array be deleted from the DataStructure", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RodriguesDataArrayPath_Key, "Input Rodrigues Vectors", "Specifies the Rodrigues data to convert", DataPath({"Cell Data", "rodrigues"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputDataArrayName_Key, "Converted Rodrigues Data Array", "The DataArray name of the converted Rodrigues vectors", "rodrigues [Converted]"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RodriguesConvertorFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType RodriguesConvertorFilter::clone() const
{
  return std::make_unique<RodriguesConvertorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RodriguesConvertorFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                 const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pRodriguesDataArrayPathValue = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  auto pOutputDataArrayPathValue = pRodriguesDataArrayPathValue.replaceName(filterArgs.value<std::string>(k_OutputDataArrayName_Key));

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& quats = dataStructure.getDataRefAs<Float32ArrayType>(pRodriguesDataArrayPathValue);

  {
    auto createConvertedQuatAction = std::make_unique<CreateArrayAction>(DataType::float32, quats.getTupleShape(), std::vector<usize>{4}, pOutputDataArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createConvertedQuatAction));
  }

  auto pRemoveOriginalArray = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  if(pRemoveOriginalArray)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pRodriguesDataArrayPathValue));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RodriguesConvertorFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  RodriguesConvertorInputValues inputValues;

  inputValues.rodrigues_data_array_path = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  inputValues.output_data_array_path = inputValues.rodrigues_data_array_path.replaceName(filterArgs.value<std::string>(k_OutputDataArrayName_Key));
  inputValues.delete_original_data = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return RodriguesConvertor(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_RodriguesDataArrayPathKey = "RodriguesDataArrayPath";
constexpr StringLiteralType k_OutputDataArrayPathKey = "OutputDataArrayPath";
constexpr StringLiteralType k_DeleteOriginalDataKey = "DeleteOriginalData";
} // namespace simpl
} // namespace

Result<Arguments> RodriguesConvertorFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RodriguesConvertorFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_RodriguesDataArrayPathKey, k_RodriguesDataArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_OutputDataArrayPathKey, k_OutputDataArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_DeleteOriginalDataKey, k_DeleteOriginalData_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
