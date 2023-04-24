#include "RodriguesConvertorFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/RodriguesConvertor.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;
namespace
{
inline constexpr int32 k_IncorrectInputArray = -7300;
inline constexpr int32 k_MissingInputArray = -7301;
} // namespace

namespace complex
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
  return {"Processing", "Crystallography", "Convert"};
}

//------------------------------------------------------------------------------
Parameters RodriguesConvertorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RodriguesDataArrayPath_Key, "", "Specifies the Rodrigues data to convert", DataPath({"CellData", "rodrigues"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputDataArrayPath_Key, "Converted Rodrigues Data Array", "", "rodrigues [Converted]"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RodriguesConvertorFilter::clone() const
{
  return std::make_unique<RodriguesConvertorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RodriguesConvertorFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pRodriguesDataArrayPathValue = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  auto pOutputDataArrayPathValue = pRodriguesDataArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Validate the Rodrigues array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pRodriguesDataArrayPathValue);
  if(quats.getNumberOfComponents() != 3)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Rodrigues Input Array must be a 3 component Float32 array"}})};
  }

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
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RodriguesConvertorFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  RodriguesConvertorInputValues inputValues;

  inputValues.RodriguesDataArrayPath = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  inputValues.OutputDataArrayPath = inputValues.RodriguesDataArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return RodriguesConvertor(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
